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
#include "nwl_private.h"

#include "nwl_defines.h"

#define NWL_BUG_EXTRA_DATA


int dma_nwl_start_loopback(nwl_dma_t *ctx,  pcilib_dma_direction_t direction, size_t packet_size) {
    uint32_t val;

    ctx->loopback_started = 1;
    dma_nwl_stop_loopback(ctx);
    
    val = packet_size;
    nwl_write_register(val, ctx, ctx->base_addr, PKT_SIZE_ADDRESS);

    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) {
	switch (direction) {
          case PCILIB_DMA_BIDIRECTIONAL:
	    val = LOOPBACK;
	    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
	    break;
          case PCILIB_DMA_TO_DEVICE:
	    return -1;
	  case PCILIB_DMA_FROM_DEVICE:
    	    val = PKTGENR;
	    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
	    break;
	}
    }

    ctx->loopback_started = 1;
        
    return 0;
}

int dma_nwl_stop_loopback(nwl_dma_t *ctx) {
    uint32_t val = 0;
    
    if (!ctx->loopback_started) return 0;

	/* Stop in any case, otherwise we can have problems in benchmark due to
	engine initialized in previous run, and benchmark is only actual usage.
	Otherwise, we should detect current loopback status during initialization */

    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) {
	nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
	nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    }
    
    ctx->loopback_started = 0;
    
    return 0;
}

double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    int iter, i;
    int err;
    size_t bytes, rbytes;
    uint32_t *buf, *cmp;
    const char *error = NULL;
    size_t packet_size, blocks;    

    size_t us = 0;
    struct timeval start, cur;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_dma_engine_t readid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_FROM_DEVICE, dma);
    pcilib_dma_engine_t writeid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_TO_DEVICE, dma);

    if (size%sizeof(uint32_t)) size = 1 + size / sizeof(uint32_t);
    else size /= sizeof(uint32_t);

	// Not supported
    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) {
	if (direction == PCILIB_DMA_TO_DEVICE) return -1.;
    }
//    else if ((direction == PCILIB_DMA_FROM_DEVICE)&&(ctx->type != PCILIB_DMA_MODIFICATION_DEFAULT)) return -1.;

	// Stop Generators and drain old data
    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) dma_nwl_stop_loopback(ctx);
//    dma_nwl_stop_engine(ctx, readid); // DS: replace with something better

    __sync_synchronize();

    err = pcilib_skip_dma(ctx->pcilib, readid);
    if (err) {
	pcilib_error("Can't start benchmark, devices continuously writes unexpected data using DMA engine");
	return err;
    }

#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_enable_engine_irq(ctx, readid);
    dma_nwl_enable_engine_irq(ctx, writeid);
#endif /* NWL_GENERATE_DMA_IRQ */

    if (size * sizeof(uint32_t) > NWL_MAX_PACKET_SIZE) {
	packet_size = NWL_MAX_PACKET_SIZE;
	blocks = (size * sizeof(uint32_t)) / packet_size + (((size*sizeof(uint32_t))%packet_size)?1:0);
    } else {
	packet_size = size * sizeof(uint32_t);
	blocks = 1;
    }

    dma_nwl_start_loopback(ctx, direction, packet_size);

	// Allocate memory and prepare data
    buf = malloc(blocks * packet_size * sizeof(uint32_t));
    cmp = malloc(blocks * packet_size * sizeof(uint32_t));
    if ((!buf)||(!cmp)) {
	if (buf) free(buf);
	if (cmp) free(cmp);
	return -1;
    }

    if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e5);
	usleep(100000);
	pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e1);

	    // This way causes more problems with garbage
	//pcilib_write_register(ctx->pcilib, NULL, "control", 0x3e1);
    }

	// Benchmark
    for (iter = 0; iter < iterations; iter++) {
        memset(cmp, 0x13 + iter, size * sizeof(uint32_t));

	if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	    pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e1);
	}

	if ((direction&PCILIB_DMA_TO_DEVICE)||(ctx->type != PCILIB_DMA_MODIFICATION_DEFAULT)) {
	    memcpy(buf, cmp, size * sizeof(uint32_t));

    	    if (direction&PCILIB_DMA_TO_DEVICE) {
		gettimeofday(&start, NULL);
	    }
	    
	    err = pcilib_write_dma(ctx->pcilib, writeid, addr, size * sizeof(uint32_t), buf, &bytes);
	    if ((err)||(bytes != size * sizeof(uint32_t))) {
		error = "Write failed";
	    	break;
	    }
	    
    	    if (direction&PCILIB_DMA_TO_DEVICE) {
		// wait written
		if (direction == PCILIB_DMA_TO_DEVICE) {
		    dma_nwl_wait_completion(ctx, writeid, PCILIB_DMA_TIMEOUT);
		}
		gettimeofday(&cur, NULL);
	        us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));    
	    }
	}

	if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	    pcilib_write_register(ctx->pcilib, NULL, "control", 0x3e1);
	}

	memset(buf, 0, size * sizeof(uint32_t));

        if (direction&PCILIB_DMA_FROM_DEVICE) {
	    gettimeofday(&start, NULL);
	}

	for (i = 0, bytes = 0; i < blocks; i++) {
#ifdef NWL_BUG_EXTRA_DATA
	    retry:
#endif
    
	    err = pcilib_read_dma(ctx->pcilib, readid, addr, packet_size * sizeof(uint32_t), buf + (bytes>>2), &rbytes);
	    if ((err)||(rbytes%sizeof(uint32_t))) {
		break;
	    } 
#ifdef NWL_BUG_EXTRA_DATA
	    else if (rbytes == 8) {
		goto retry;	
	    }
#endif
	    bytes += rbytes;
	}

        if (direction&PCILIB_DMA_FROM_DEVICE) {
	    gettimeofday(&cur, NULL);
	    us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));
	}
#ifdef NWL_BUG_EXTRA_DATA
	if ((err)||((bytes != size * sizeof(uint32_t))&&((bytes - 8) != size * sizeof(uint32_t)))) {
#else
	if ((err)||(bytes != size * sizeof(uint32_t))) {
#endif
	    printf("Expected: %zu bytes, but %zu read, error: %i\n", size * sizeof(uint32_t), bytes, err);
	    error = "Read failed";
	    break;
	}
	
#ifndef NWL_BUG_EXTRA_DATA
	if (direction == PCILIB_DMA_BIDIRECTIONAL) {
	    if (memcmp(buf, cmp, size * sizeof(uint32_t))) {
		for (i = 0; i < size; i++)
		    if (buf[i] != cmp[i]) break;
		
		bytes = i;
		printf("Expected: *0x%lx, Written at dword %lu:", 0x13 + iter, bytes);
		for (; (i < size)&&(i < (bytes + 16)); i++) {
		    if (((i - bytes)%8)==0) printf("\n");
		    printf("% 10lx", buf[i]);
		}
		printf("\n");
		
		error = "Written and read values does not match";
		break;
	    }
	}
#endif
    }

    if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e1);
    }

    if (error) {
	pcilib_warning("%s at iteration %i, error: %i, bytes: %zu", error, iter, err, bytes);
    }
    
#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_disable_engine_irq(ctx, writeid);
    dma_nwl_disable_engine_irq(ctx, readid);
#endif /* NWL_GENERATE_DMA_IRQ */

    dma_nwl_stop_loopback(ctx);

    __sync_synchronize();
    
    if (direction == PCILIB_DMA_FROM_DEVICE) {
	pcilib_skip_dma(ctx->pcilib, readid);
    }
    
    free(cmp);
    free(buf);

    return /*error?-1:*/(1. * size * sizeof(uint32_t) * iterations * 1000000) / (1024. * 1024. * us);
}
