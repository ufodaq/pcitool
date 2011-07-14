#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "pci.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "nwl.h"

#include "nwl_defines.h"

#include "nwl_buffers.h"    

int dma_nwl_read_engine_config(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, char *base) {
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

	// This will only successed if there are no parallel access to DMA engine
    err = dma_nwl_allocate_engine_buffers(ctx, info);
    if (err) return err;
    
	// Check if DMA engine is enabled
    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_CTRL_STATUS);
    if (val&DMA_ENG_RUNNING) {	
//	info->preserve = 1;
	
	// We need to positionate buffers correctly (both read and write)
	//DSS info->tail, info->head
    
//	pcilib_error("Not implemented");
	
//        info->started = 1;
//	return 0;
    }

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

    if (!info->preserve) {
	    // Stopping DMA is not enough reset is required
	val = DMA_ENG_DISABLE|DMA_ENG_USER_RESET|DMA_ENG_RESET;
	nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);

	if (info->ring) {
	    ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);
	    nwl_write_register(ring_pa, ctx, info->base_addr, REG_DMA_ENG_NEXT_BD);
	    nwl_write_register(ring_pa, ctx, info->base_addr, REG_SW_NEXT_BD);
	}
    }

	// Acknowledge asserted engine interrupts    
    if (val & DMA_ENG_INT_ACTIVE_MASK) {
	val |= DMA_ENG_ALLINT_MASK;
	nwl_write_register(val, ctx, base, REG_DMA_ENG_CTRL_STATUS);
    }

	// Clean buffers
    if (info->ring) {
	pcilib_free_kernel_memory(ctx->pcilib, info->ring, info->preserve?PCILIB_KMEM_FLAG_REUSE:0);
	info->ring = NULL;
    }

    if (info->pages) {
	pcilib_free_kernel_memory(ctx->pcilib, info->pages, info->preserve?PCILIB_KMEM_FLAG_REUSE:0);
	info->pages = NULL;
    }

    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_CTRL_STATUS);

    return 0;
}

int dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *data, size_t *written) {
    int err;
    size_t pos;
    size_t bufnum;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start(ctx, dma, PCILIB_DMA_FLAGS_DEFAULT);
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
    
    return 0;
}

int dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err, ret;
    size_t res = 0;
    size_t bufnum;
    size_t bufsize;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    size_t buf_size;
    int eop;

    pcilib_nwl_engine_description_t *info = ctx->engines + dma;

    err = dma_nwl_start(ctx, dma, PCILIB_DMA_FLAGS_DEFAULT);
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