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
#include "nwl_register.h"


#define NWL_XAUI_ENGINE 0
#define NWL_XRAWDATA_ENGINE 1
#define NWL_FIX_EOP_FOR_BIG_PACKETS		// requires precise sizes in read requests
#define NWL_GENERATE_DMA_IRQ

#define PCILIB_NWL_ALIGNMENT 			64  // in bytes
#define PCILIB_NWL_DMA_DESCRIPTOR_SIZE		64  // in bytes
#define PCILIB_NWL_DMA_PAGES			512 // 1024


static int dma_nwl_read_engine_config(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, char *base) {
    uint32_t val;
    
    info->base_addr = base;
    
    nwl_read_register(val, ctx, base, REG_DMA_ENG_CAP);

    if ((val & DMA_ENG_PRESENT_MASK) == 0) return PCILIB_ERROR_NOTAVAILABLE;
    
    info->desc.addr = (val & DMA_ENG_NUMBER) >> DMA_ENG_NUMBER_SHIFT;
    if ((info->desc.addr > PCILIB_MAX_DMA_ENGINES)||(info->desc.addr < 0)) return PCILIB_ERROR_INVALID_DATA;
    
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

    info->base_addr = base;
    
    return 0;
}


int dma_nwl_start_engine(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    int err;
    uint32_t val;
    uint32_t ring_pa;
    struct timeval start, cur;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;
    char *base = ctx->engines[dma].base_addr;

    if (info->started) return 0;

	// Disable IRQs
    err = dma_nwl_disable_engine_irq(ctx, dma);
    if (err) return err;

	// Disable Engine & Reseting 
    val = DMA_ENG_DISABLE|DMA_ENG_USER_RESET;
    nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);

    gettimeofday(&start, NULL);
    do {
	nwl_read_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
        gettimeofday(&cur, NULL);
    } while ((val & (DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET))&&(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < PCILIB_REGISTER_TIMEOUT));
    
    if (val & (DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET)) {
	pcilib_error("Timeout during reset of DMA engine %i", info->desc.addr);
	return PCILIB_ERROR_TIMEOUT;
    }

    val = DMA_ENG_RESET; 
    nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    
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

    err = dma_nwl_start(ctx);
    if (err) return err;
    
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

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_engine_irq(ctx, dma);
#endif /* NWL_GENERATE_DMA_IRQ */

    if (info->desc.direction == PCILIB_DMA_FROM_DEVICE) {
	ring_pa += (info->ring_size - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    	nwl_write_register(ring_pa, ctx, info->base_addr, REG_SW_NEXT_BD);

	info->tail = 0;
	info->head = (info->ring_size - 1);
    } else {
	info->tail = 0;
	info->head = 0;
    }
    
    info->started = 1;
    
    return 0;
}


int dma_nwl_stop_engine(nwl_dma_t *ctx, pcilib_dma_engine_t dma) {
    int err;
    uint32_t val;
    uint32_t ring_pa;
    struct timeval start, cur;
    
    pcilib_nwl_engine_description_t *info = ctx->engines + dma;
    char *base = ctx->engines[dma].base_addr;

    info->started = 0;

    err = dma_nwl_disable_engine_irq(ctx, dma);
    if (err) return err;

    val = DMA_ENG_DISABLE; 
    nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);

    if (info->ring) {
	ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);
	nwl_write_register(ring_pa, ctx, info->base_addr, REG_DMA_ENG_NEXT_BD);
	nwl_write_register(ring_pa, ctx, info->base_addr, REG_SW_NEXT_BD);
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



    return 0;
}


int dma_nwl_start_loopback(nwl_dma_t *ctx,  pcilib_dma_direction_t direction, size_t packet_size) {
    uint32_t val;
    
    val = packet_size;
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
    
    return 0;
}

int dma_nwl_stop_loopback(nwl_dma_t *ctx) {
    uint32_t val;

    val = 0;
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    
    return 0;
}


int dma_nwl_start(nwl_dma_t *ctx) {
    if (ctx->started) return 0;
    
#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_irq(ctx, PCILIB_DMA_IRQ);
#endif /* NWL_GENERATE_DMA_IRQ */

    ctx->started = 1;

    return 0;
}

int dma_nwl_stop(nwl_dma_t *ctx) {
    int err;

    pcilib_dma_engine_t i;
    
    ctx->started = 0;
    
    err = dma_nwl_disable_irq(ctx);
    if (err) return err;
    
    err = dma_nwl_stop_loopback(ctx);
    if (err) return err;
    
    for (i = 0; i < ctx->n_engines; i++) {
	err = dma_nwl_stop_engine(ctx, i);
	if (err) return err;
    }
    
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
	    free(ctx);
	    pcilib_error("DMA Register Bank could not be found");
	    return NULL;
	}
	
	ctx->dma_bank = model_info->banks + dma_bank;
	ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);

	for (i = 0, n_engines = 0; i < 2 * PCILIB_MAX_DMA_ENGINES; i++) {
	    char *addr = ctx->base_addr + DMA_OFFSET + i * DMA_ENGINE_PER_SIZE;

	    memset(ctx->engines + n_engines, 0, sizeof(pcilib_nwl_engine_description_t));

	    err = dma_nwl_read_engine_config(ctx, ctx->engines + n_engines, addr);
	    if (err) continue;
	    
	    pcilib_set_dma_engine_description(pcilib, n_engines, (pcilib_dma_engine_description_t*)(ctx->engines + n_engines));
	    ++n_engines;
	}
	pcilib_set_dma_engine_description(pcilib, n_engines, NULL);
	
	ctx->n_engines = n_engines;
	
	err = nwl_add_registers(ctx);
	if (err) {
	    free(ctx);
	    pcilib_error("Failed to add DMA registers");
	    return NULL;
	}
    }
    return (pcilib_dma_context_t*)ctx;
}

void  dma_nwl_free(pcilib_dma_context_t *vctx) {
    pcilib_dma_engine_t i;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    if (ctx) {
	for (i = 0; i < ctx->n_engines; i++) dma_nwl_stop_engine(ctx, i);
	dma_nwl_stop(ctx);
	free(ctx);
    }
}

int dma_nwl_sync_buffers(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, pcilib_kmem_handle_t *kmem) {
    switch (info->desc.direction) {
     case PCILIB_DMA_FROM_DEVICE:
        return pcilib_sync_kernel_memory(ctx->pcilib, kmem, PCILIB_KMEM_SYNC_FROMDEVICE);
     case PCILIB_DMA_TO_DEVICE:
        return pcilib_sync_kernel_memory(ctx->pcilib, kmem, PCILIB_KMEM_SYNC_TODEVICE);
    }
    
    return 0;
}

#include "nwl_buffers.h"    

int dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, void *data, size_t *written) {
    int err;
    size_t pos;
    size_t bufnum;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start_engine(ctx, dma);
    if (err) return err;

    if (data) {
	for (pos = 0; pos < size; pos += info->page_size) {
	    int block_size = min2(size - pos, info->page_size);
    
    	    bufnum = dma_nwl_get_next_buffer(ctx, info, 1, timeout);
	    if (bufnum == PCILIB_DMA_BUFFER_INVALID) {
		if (written) *written = pos;
		return PCILIB_ERROR_TIMEOUT;
	    }
	
    	    void *buf = pcilib_kmem_get_block_ua(ctx->pcilib, info->pages, bufnum);
	    memcpy(buf, data, block_size);

	    err = dma_nwl_push_buffer(ctx, info, block_size, (flags&PCILIB_DMA_FLAG_EOP)&&((pos + block_size) == size), timeout);
	    if (err) {
		if (written) *written = pos;
		return err;
	    }
	}    
    }
    
    if (written) *written = size;
    
    if (flags&PCILIB_DMA_FLAG_WAIT) {
	bufnum =  dma_nwl_get_next_buffer(ctx, info, PCILIB_NWL_DMA_PAGES - 1, timeout);
	if (bufnum == PCILIB_DMA_BUFFER_INVALID) return PCILIB_ERROR_TIMEOUT;
    }
    
    return size;
}

int dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err, ret;
    size_t res = 0;
    size_t bufnum;
    size_t bufsize;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    size_t buf_size;
    int eop;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start_engine(ctx, dma);
    if (err) return err;

    do {
        bufnum = dma_nwl_wait_buffer(ctx, info, &bufsize, &eop, timeout);
	if (bufnum == PCILIB_DMA_BUFFER_INVALID) return PCILIB_ERROR_TIMEOUT;

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

    } while (ret);
    
    return 0;
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

    char *read_base = ctx->engines[readid].base_addr;
    char *write_base = ctx->engines[writeid].base_addr;

    if (size%sizeof(uint32_t)) size = 1 + size / sizeof(uint32_t);
    else size /= sizeof(uint32_t);

	// Stop Generators and drain old data
    dma_nwl_stop_loopback(ctx);

    __sync_synchronize();

    pcilib_skip_dma(ctx->pcilib, readid);

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_engine_irq(ctx, readid);
#endif /* NWL_GENERATE_DMA_IRQ */
    
    dma_nwl_start_loopback(ctx, direction, size * sizeof(uint32_t));

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
        gettimeofday(&start, NULL);
	if (direction&PCILIB_DMA_TO_DEVICE) {
	    memcpy(buf, cmp, size * sizeof(uint32_t));

	    err = pcilib_write_dma(ctx->pcilib, writeid, addr, size * sizeof(uint32_t), buf, &bytes);
	    if ((err)||(bytes != size * sizeof(uint32_t))) {
		error = "Write failed";
	        break;
	    }
	}

	memset(buf, 0, size * sizeof(uint32_t));
        
	err = pcilib_read_dma(ctx->pcilib, readid, addr, size * sizeof(uint32_t), buf, &bytes);
        gettimeofday(&cur, NULL);
	us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));

	if ((err)||(bytes != size * sizeof(uint32_t))) {
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

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_disable_irq(ctx);
#endif /* NWL_GENERATE_DMA_IRQ */

    dma_nwl_stop_loopback(ctx);

    __sync_synchronize();
    
    if (direction == PCILIB_DMA_FROM_DEVICE) {
	pcilib_skip_dma(ctx->pcilib, readid);
    }
    
    free(cmp);
    free(buf);

    return error?-1:(1. * size * sizeof(uint32_t) * iterations * 1000000) / (1024. * 1024. * us);
}
