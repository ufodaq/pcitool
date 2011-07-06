#define _PCILIB_DMA_NWL_C
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "pci.h"
#include "dma.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "nwl.h"

#include "nwl_defines.h"


#define NWL_XAUI_ENGINE 0
#define NWL_XRAWDATA_ENGINE 1
#define NWL_FIX_EOP_FOR_BIG_PACKETS		// requires precise sizes in read requests

/*
pcilib_register_bank_description_t ipecamera_register_banks[] = {
    { PCILIB_REGISTER_DMABANK0, PCILIB_BAR0, 128, PCILIB_DEFAULT_PROTOCOL, DMA_NWL_OFFSET, DMA_NWL_OFFSET, PCILIB_LITTLE_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "%lx", "dma", "NorthWest Logick DMA Engine" },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

pcilib_register_description_t dma_nwl_registers[] = {
    {0, 	0, 	32, 	0, 	PCILIB_REGISTER_R , PCILIB_REGISTER_DMABANK, "dma_capabilities",  ""},
    {1, 	0, 	32, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_DMABANK, "dma_control",  ""},
};
*/

typedef struct {
    pcilib_dma_engine_description_t desc;
    char *base_addr;
    
    size_t ring_size, page_size;
    size_t head, tail;
    pcilib_kmem_handle_t *ring;
    pcilib_kmem_handle_t *pages;
    
    int started;			// indicates if DMA buffers are initialized and reading is allowed
    int writting;			// indicates if we are in middle of writting packet

} pcilib_nwl_engine_description_t;


struct nwl_dma_s {
    pcilib_t *pcilib;
    
    pcilib_register_bank_description_t *dma_bank;
    char *base_addr;
    
    pcilib_dma_engine_t n_engines;
    pcilib_nwl_engine_description_t engines[PCILIB_MAX_DMA_ENGINES + 1];
};

#define nwl_read_register(var, ctx, base, reg) pcilib_datacpy(&var, base + reg, 4, 1, ctx->dma_bank->raw_endianess)
#define nwl_write_register(var, ctx, base, reg) pcilib_datacpy(base + reg, &var, 4, 1, ctx->dma_bank->raw_endianess)

static int nwl_read_engine_config(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, char *base) {
    uint32_t val;
    
    info->base_addr = base;
    
    nwl_read_register(val, ctx, base, REG_DMA_ENG_CAP);

    if ((val & DMA_ENG_PRESENT_MASK) == 0) return PCILIB_ERROR_NOTAVAILABLE;
    
    info->desc.addr = (val & DMA_ENG_NUMBER) >> DMA_ENG_NUMBER_SHIFT;
    
    switch (val & DMA_ENG_DIRECTION_MASK) {
	case  DMA_ENG_C2S:
	    info->desc.direction = PCILIB_DMA_FROM_DEVICE;
	break;
	default:
	    info->desc.direction = PCILIB_DMA_TO_DEVICE;
    }
    
    switch (val & DMA_ENG_TYPE_MASK) {
	case DMA_ENG_BLOCK:
	    info->desc.type = PCILIB_DMA_TYPE_BLOCK;
	break;
	case DMA_ENG_PACKET:
	    info->desc.type = PCILIB_DMA_TYPE_PACKET;
	break;
	default:
	    info->desc.type = PCILIB_DMA_TYPE_UNKNOWN;
    }
    
    info->desc.addr_bits = (val & DMA_ENG_BD_MAX_BC) >> DMA_ENG_BD_MAX_BC_SHIFT;
    
    return 0;
}

static int nwl_stop_engine(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    uint32_t val;
    struct timeval start, cur;
    
    pcilib_nwl_engine_description_t *info = ctx->engines + dma;
    char *base = ctx->engines[dma].base_addr;

    if (info->desc.addr == NWL_XRAWDATA_ENGINE) {
	    // Stop Generators
	nwl_read_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
	val = ~(LOOPBACK|PKTCHKR|PKTGENR);
	nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);

	nwl_read_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
	val = ~(LOOPBACK|PKTCHKR|PKTGENR);
	nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);

	    // Skip everything in read queue (could be we need to start and skip as well)
	if (info->started) pcilib_skip_dma(ctx->pcilib, dma);
    }

	// Disable IRQ
    nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    val &= ~(DMA_ENG_INT_ENABLE);
    nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);

	// Reseting 
    val = DMA_ENG_DISABLE|DMA_ENG_USER_RESET; nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    gettimeofday(&start, NULL);
    do {
	nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
        gettimeofday(&cur, NULL);
    } while ((val & (DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET))&&(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < PCILIB_REGISTER_TIMEOUT));
    
    if (val & (DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET)) {
	pcilib_error("Timeout during reset of DMA engine %i", info->desc.addr);
	return PCILIB_ERROR_TIMEOUT;
    }

    val = DMA_ENG_RESET; nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    gettimeofday(&start, NULL);
    do {
	nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
        gettimeofday(&cur, NULL);
    } while ((val & DMA_ENG_RESET)&&(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < PCILIB_REGISTER_TIMEOUT));
    
    if (val & DMA_ENG_RESET) {
	pcilib_error("Timeout during reset of DMA engine %i", info->desc.addr);
	return PCILIB_ERROR_TIMEOUT;
    }

	// Acknowledge asserted engine interrupts    
    if (val & DMA_ENG_INT_ACTIVE_MASK) {
	val |= DMA_ENG_ALLINT_MASK;
	nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    }
    
	// Clean buffers
    if (info->ring) {
	pcilib_free_kernel_memory(ctx->pcilib, info->ring);
	info->ring = NULL;
    }

    if (info->pages) {
	pcilib_free_kernel_memory(ctx->pcilib, info->pages);
	info->pages = NULL;
    }
    
    info->started = 0;

    return 0;
}

pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib) {
    int i;
    int err;
    uint32_t val;
    pcilib_dma_engine_t n_engines;

    pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (ctx) {
	memset(ctx, 0, sizeof(nwl_dma_t));
	ctx->pcilib = pcilib;
	pcilib_register_bank_t dma_bank = pcilib_find_bank_by_addr(pcilib, PCILIB_REGISTER_BANK_DMA);

	if (dma_bank == PCILIB_REGISTER_BANK_INVALID) {
	    pcilib_error("DMA Register Bank could not be found");
	    return NULL;
	}
	
	ctx->dma_bank = model_info->banks + dma_bank;
	ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);

        val = 0;
	nwl_read_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
	nwl_read_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);

	for (i = 0, n_engines = 0; i < 2 * PCILIB_MAX_DMA_ENGINES; i++) {
	    char *addr = ctx->base_addr + DMA_OFFSET + i * DMA_ENGINE_PER_SIZE;

	    memset(ctx->engines + n_engines, 0, sizeof(pcilib_nwl_engine_description_t));

	    err = nwl_read_engine_config(ctx, ctx->engines + n_engines, addr);
	    if (!err) err = nwl_stop_engine(ctx, n_engines);
	    if (!err) {
		ctx->engines[n_engines].base_addr = addr;
		pcilib_set_dma_engine_description(pcilib, n_engines, (pcilib_dma_engine_description_t*)(ctx->engines + n_engines));
		++n_engines;
	    }
	    
	}
	pcilib_set_dma_engine_description(pcilib, n_engines, NULL);
	
	ctx->n_engines = n_engines;
    }
    return (pcilib_dma_context_t*)ctx;
}

void  dma_nwl_free(pcilib_dma_context_t *vctx) {
    pcilib_dma_engine_t i;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    if (ctx) {
	for (i = 0; i < ctx->n_engines; i++) nwl_stop_engine(vctx, i);
	free(ctx);
    }
}

#define PCILIB_NWL_ALIGNMENT 			64  // in bytes
#define PCILIB_NWL_DMA_DESCRIPTOR_SIZE		64  // in bytes
#define PCILIB_NWL_DMA_PAGES			512 // 1024

#define NWL_RING_GET(data, offset)  *(uint32_t*)(((char*)(data)) + (offset))
#define NWL_RING_SET(data, offset, val)  *(uint32_t*)(((char*)(data)) + (offset)) = (val)
#define NWL_RING_UPDATE(data, offset, mask, val) *(uint32_t*)(((char*)(data)) + (offset)) = ((*(uint32_t*)(((char*)(data)) + (offset)))&(mask))|(val)


int dma_nwl_sync_buffers(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, pcilib_kmem_handle_t *kmem) {
    switch (info->desc.direction) {
     case PCILIB_DMA_FROM_DEVICE:
        return pcilib_sync_kernel_memory(ctx->pcilib, kmem, PCILIB_KMEM_SYNC_FROMDEVICE);
     case PCILIB_DMA_TO_DEVICE:
        return pcilib_sync_kernel_memory(ctx->pcilib, kmem, PCILIB_KMEM_SYNC_TODEVICE);
    }
    
    return 0;
}

int dma_nwl_allocate_engine_buffers(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info) {
    int err = 0;

    int i;
    uint32_t val;
    uint32_t buf_sz;
    uint64_t buf_pa;

    char *base = info->base_addr;
    
    if (info->pages) return 0;
    
    pcilib_kmem_handle_t *ring = pcilib_alloc_kernel_memory(ctx->pcilib, PCILIB_KMEM_TYPE_CONSISTENT, 1, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE, PCILIB_NWL_ALIGNMENT, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA, info->desc.addr), 0);
    pcilib_kmem_handle_t *pages = pcilib_alloc_kernel_memory(ctx->pcilib, PCILIB_KMEM_TYPE_PAGE, PCILIB_NWL_DMA_PAGES, 0, PCILIB_NWL_ALIGNMENT, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA, info->desc.addr), 0);

    if ((ring)&&(pages)) err = dma_nwl_sync_buffers(ctx, info, pages);
    else err = PCILIB_ERROR_FAILED;


    if (err) {
	if (pages) pcilib_free_kernel_memory(ctx->pcilib, pages);
	if (ring) pcilib_free_kernel_memory(ctx->pcilib, ring);    
	return err;
    }
    
    unsigned char *data = (unsigned char*)pcilib_kmem_get_ua(ctx->pcilib, ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, ring);
    
    memset(data, 0, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE);

    for (i = 0; i < PCILIB_NWL_DMA_PAGES; i++, data += PCILIB_NWL_DMA_DESCRIPTOR_SIZE) {
	buf_pa = pcilib_kmem_get_block_pa(ctx->pcilib, pages, i);
	buf_sz = pcilib_kmem_get_block_size(ctx->pcilib, pages, i);

	NWL_RING_SET(data, DMA_BD_NDESC_OFFSET, ring_pa + ((i + 1) % PCILIB_NWL_DMA_PAGES) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	NWL_RING_SET(data, DMA_BD_BUFAL_OFFSET, buf_pa&0xFFFFFFFF);
	NWL_RING_SET(data, DMA_BD_BUFAH_OFFSET, buf_pa>>32);
        NWL_RING_SET(data, DMA_BD_BUFL_CTRL_OFFSET, buf_sz);
/*
	if (info->desc.direction == PCILIB_DMA_TO_DEVICE) {
	    NWL_RING_SET(data, DMA_BD_BUFL_STATUS_OFFSET, buf_sz);
	}
*/
    }

    val = ring_pa;
    nwl_write_register(val, ctx, base, REG_DMA_ENG_NEXT_BD);
    nwl_write_register(val, ctx, base, REG_SW_NEXT_BD);
    
    info->ring = ring;
    info->pages = pages;
    info->page_size = buf_sz;
    info->ring_size = PCILIB_NWL_DMA_PAGES;
    
    info->head = 0;
    info->tail = 0;
    
    return 0;
}

static int dma_nwl_start(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info) {
    int err;
    uint32_t ring_pa;
    uint32_t val;

    if (info->started) return 0;
    
    err = dma_nwl_allocate_engine_buffers(ctx, info);
    if (err) return err;
    
    ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);
    nwl_write_register(ring_pa, ctx, info->base_addr, REG_DMA_ENG_NEXT_BD);
    nwl_write_register(ring_pa, ctx, info->base_addr, REG_SW_NEXT_BD);

    __sync_synchronize();

    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_CTRL_STATUS);
    val |= (DMA_ENG_ENABLE);
    nwl_write_register(val, ctx, info->base_addr, REG_DMA_ENG_CTRL_STATUS);

    __sync_synchronize();

    if (info->desc.direction == PCILIB_DMA_FROM_DEVICE) {
	ring_pa += (info->ring_size - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    	nwl_write_register(ring_pa, ctx, info->base_addr, REG_SW_NEXT_BD);
//	nwl_read_register(val, ctx, info->base_addr, 0x18);

	info->tail = 0;
	info->head = (info->ring_size - 1);
    } else {
	info->tail = 0;
	info->head = 0;
    }
    
    info->started = 1;
    
    return 0;
}

static size_t dma_nwl_clean_buffers(nwl_dma_t * ctx, pcilib_nwl_engine_description_t *info) {
    size_t res = 0;
    uint32_t status, control;

    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

next_buffer:
    status = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET)&DMA_BD_STATUS_MASK;
//  control = NWL_RING_GET(ring, DMA_BD_BUFL_CTRL_OFFSET)&DMA_BD_CTRL_MASK;
    
    if (status & DMA_BD_ERROR_MASK) {
        pcilib_error("NWL DMA Engine reported error in ring descriptor");
        return (size_t)-1;
    }
	
    if (status & DMA_BD_SHORT_MASK) {
        pcilib_error("NWL DMA Engine reported short error");
        return (size_t)-1;
    }
	
    if (status & DMA_BD_COMP_MASK) {
	info->tail++;
	if (info->tail == info->ring_size) {
	    ring -= (info->tail - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	    info->tail = 0;
	} else {
	    ring += PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	}
	
	res++;

	if (info->tail != info->head) goto next_buffer;
    }
    
//    printf("====> Cleaned: %i\n", res);
    return res;
}


static size_t dma_nwl_get_next_buffer(nwl_dma_t * ctx, pcilib_nwl_engine_description_t *info, size_t n_buffers, size_t timeout) {
    struct timeval start, cur;

    size_t res, n = 0;
    size_t head;

    for (head = info->head; (((head + 1)%info->ring_size) != info->tail)&&(n < n_buffers); head++, n++);
    if (n == n_buffers) return info->head;

    gettimeofday(&start, NULL);

    res = dma_nwl_clean_buffers(ctx, info);
    if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
    else n += res;

    
    while (n < n_buffers) {
	if (timeout != PCILIB_TIMEOUT_INFINITE) {
	    gettimeofday(&cur, NULL);
	    if  (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > timeout) break;
	}
	
	usleep (10);	

        res = dma_nwl_clean_buffers(ctx, info);
        if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
	else if (res > 0) {
	    gettimeofday(&start, NULL);
	    n += res;
	}
    }
    
    if (n < n_buffers) return PCILIB_DMA_BUFFER_INVALID;
    
    return info->head;
}

static int dma_nwl_push_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, size_t size, int eop, size_t timeout) {
    int flags;
    
    uint32_t val;
    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);

    ring += info->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    
    if (!info->writting) {
	flags |= DMA_BD_SOP_MASK;
	info->writting = 1;
    }
    if (eop) {
	flags |= DMA_BD_EOP_MASK;
	info->writting = 0;
    }

    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, size|flags);
    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, size);

    info->head++;
    if (info->head == info->ring_size) info->head = 0;
    
    val = ring_pa + info->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, info->base_addr, REG_SW_NEXT_BD);
//    nwl_read_register(val, ctx, info->base_addr, 0x18);

//    usleep(10000);

//    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_LAST_BD);
//    printf("Last BD(Write): %lx %lx\n", ring, val);
    
    
    return 0;
}


static size_t dma_nwl_wait_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, size_t *size, int *eop, size_t timeout) {
    uint32_t val;
    struct timeval start, cur;
    uint32_t status_size, status, control;

//    usleep(10000);
    
    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    
//    status_size = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
//    printf("Status0: %lx\n", status_size);

    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    gettimeofday(&start, NULL);
    
//    printf("Waiting %li\n", info->tail);
//    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_LAST_BD);
//    printf("Last BD(Read): %lx %lx\n", ring, val);

    do {
	status_size = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
	status = status_size & DMA_BD_STATUS_MASK;
	
//	printf("%i: %lx\n", info->tail, status_size);
    
	if (status & DMA_BD_ERROR_MASK) {
    	    pcilib_error("NWL DMA Engine reported error in ring descriptor");
    	    return (size_t)-1;
	}	
	
	if (status & DMA_BD_COMP_MASK) {
	    if (status & DMA_BD_EOP_MASK) *eop = 1;
	    else *eop = 0;
        
	    *size = status_size & DMA_BD_BUFL_MASK;
	
//	    printf("Status: %lx\n", status_size);
	    return info->tail;
	}
	
	usleep(10);
        gettimeofday(&cur, NULL);
    } while ((timeout == PCILIB_TIMEOUT_INFINITE)||(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < timeout));

//    printf("Final status: %lx\n", status_size);
    
    return (size_t)-1;
}

static int dma_nwl_return_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info) {
    uint32_t val;

    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);
    size_t bufsz = pcilib_kmem_get_block_size(ctx->pcilib, info->pages, info->tail);

    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
//    printf("Returning: %i\n", info->tail);

    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, bufsz);
    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, 0);

    val = ring_pa + info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, info->base_addr, REG_SW_NEXT_BD);
//    nwl_read_register(val, ctx, info->base_addr, 0x18);
    
    info->tail++;
    if (info->tail == info->ring_size) info->tail = 0;
}
    

size_t dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, void *data) {
    int err;
    size_t pos;
    size_t bufnum;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start(ctx, info);
    if (err) return 0;

    for (pos = 0; pos < size; pos += info->page_size) {
	int block_size = min2(size - pos, info->page_size);
    
        bufnum = dma_nwl_get_next_buffer(ctx, info, 1, timeout);
	if (bufnum == PCILIB_DMA_BUFFER_INVALID) return pos;
	
	    //sync
        void *buf = pcilib_kmem_get_block_ua(ctx->pcilib, info->pages, bufnum);
	memcpy(buf, data, block_size);

	err = dma_nwl_push_buffer(ctx, info, block_size, (flags&PCILIB_DMA_FLAG_EOP)&&((pos + block_size) == size), timeout);
	if (err) return pos;
    }    
    
    return size;
}

size_t dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err, ret;
    size_t res = 0;
    size_t bufnum;
    size_t bufsize;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    size_t buf_size;
    int eop;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start(ctx, info);
    if (err) return 0;

    do {
        bufnum = dma_nwl_wait_buffer(ctx, info, &bufsize, &eop, timeout);
	if (bufnum == PCILIB_DMA_BUFFER_INVALID) return 0;

#ifdef NWL_FIX_EOP_FOR_BIG_PACKETS
	if (size > 65536) {
//	    printf("%i %i\n", res + bufsize, size);
	    if ((res+bufsize) < size) eop = 0;
	    else if ((res+bufsize) == size) eop = 1;
	}
#endif /*  NWL_FIX_EOP_FOR_BIG_PACKETS */
	
	//sync
        void *buf = pcilib_kmem_get_block_ua(ctx->pcilib, info->pages, bufnum);
	ret = cb(cbattr, eop?PCILIB_DMA_FLAG_EOP:0, bufsize, buf);
	dma_nwl_return_buffer(ctx, info);
	
	res += bufsize;
	
//	printf("%i %i %i (%li)\n", ret, res, eop, size);
    } while (ret);
    
    return res;
}

double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    int i;
    int res;
    int err;
    size_t bytes;
    uint32_t val;
    uint32_t *buf, *cmp;
    const char *error = NULL;

    size_t us = 0;
    struct timeval start, cur;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_dma_engine_t readid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_FROM_DEVICE, dma);
    pcilib_dma_engine_t writeid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_TO_DEVICE, dma);

    if (size%sizeof(uint32_t)) size = 1 + size / sizeof(uint32_t);
    else size /= sizeof(uint32_t);


	// Stop Generators and drain old data
    nwl_read_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    val = ~(LOOPBACK|PKTCHKR|PKTGENR);
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);

    nwl_read_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    val = ~(LOOPBACK|PKTCHKR|PKTGENR);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);

/*
    nwl_stop_engine(ctx, readid);
    nwl_stop_engine(ctx, writeid);

    err = dma_nwl_start(ctx, ctx->engines + readid);
    if (err) return -1;
    err = dma_nwl_start(ctx, ctx->engines + writeid);
    if (err) return -1;
*/

    __sync_synchronize();

    pcilib_skip_dma(ctx->pcilib, readid);


	// Set size and required mode
    val = size * sizeof(uint32_t);
    nwl_write_register(val, ctx, ctx->base_addr, PKT_SIZE_ADDRESS);

    switch (direction) {
      case PCILIB_DMA_BIDIRECTIONAL:
	val = LOOPBACK;
	break;
      case PCILIB_DMA_TO_DEVICE:
	return -1;
      case PCILIB_DMA_FROM_DEVICE:
        val = PKTGENR;
	break;
    }

    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);


	// Allocate memory and prepare data
    buf = malloc(size * sizeof(uint32_t));
    cmp = malloc(size * sizeof(uint32_t));
    if ((!buf)||(!cmp)) {
	if (buf) free(buf);
	if (cmp) free(cmp);
	return -1;
    }

    memset(cmp, 0x13, size * sizeof(uint32_t));

	// Benchmark
    for (i = 0; i < iterations; i++) {
//	printf("Iteration: %i\n", i);

        gettimeofday(&start, NULL);
	if (direction&PCILIB_DMA_TO_DEVICE) {
	    memcpy(buf, cmp, size * sizeof(uint32_t));

	    bytes = pcilib_write_dma(ctx->pcilib, writeid, addr, size * sizeof(uint32_t), buf);
	    if (bytes != size * sizeof(uint32_t)) {
		error = "Write failed";
	        break;
	    }
	}

	memset(buf, 0, size * sizeof(uint32_t));
        
	bytes = pcilib_read_dma(ctx->pcilib, readid, addr, size * sizeof(uint32_t), buf);
        gettimeofday(&cur, NULL);
	us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));


	if (bytes != size * sizeof(uint32_t)) {
	     printf("RF: %li %li\n", bytes, size * 4);
	     error = "Read failed";
	     break;
	}
	
	if (direction == PCILIB_DMA_BIDIRECTIONAL) {
	    res = memcmp(buf, cmp, size * sizeof(uint32_t));
	    if (res) {
		error = "Written and read values does not match";
		break;
	    }
	}
	     
    }


	// Stop Generators and drain data if necessary
    nwl_read_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    val = ~(LOOPBACK|PKTCHKR|PKTGENR);
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);

    nwl_read_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    val = ~(LOOPBACK|PKTCHKR|PKTGENR);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);

    __sync_synchronize();
    
    if (direction == PCILIB_DMA_FROM_DEVICE) {
	pcilib_skip_dma(ctx->pcilib, readid);
    }
    
    free(cmp);
    free(buf);

    return error?-1:(1. * size * sizeof(uint32_t) * iterations * 1000000) / (1024. * 1024. * us);
}
