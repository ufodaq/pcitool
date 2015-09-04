#define _PCILIB_DMA_IPE_C
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "pci.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "debug.h"

#include "ipe.h"
#include "ipe_private.h"


typedef struct {
    size_t size;
    size_t pos;
    pcilib_dma_flags_t flags;
} dma_ipe_skim_callback_context_t;

static int dma_ipe_skim_callback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    dma_ipe_skim_callback_context_t *ctx = (dma_ipe_skim_callback_context_t*)arg;

    ctx->pos += bufsize;

    if (flags & PCILIB_DMA_FLAG_EOP) {
	if ((ctx->pos < ctx->size)&&(ctx->flags&PCILIB_DMA_FLAG_MULTIPACKET)) {
	    if (ctx->flags&PCILIB_DMA_FLAG_WAIT) return PCILIB_STREAMING_WAIT;
	    else return PCILIB_STREAMING_CONTINUE;
	}
	return PCILIB_STREAMING_STOP;
    }

    return PCILIB_STREAMING_REQ_FRAGMENT;
}

int dma_ipe_skim_dma_custom(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *read_bytes) {
    int err; 

    dma_ipe_skim_callback_context_t opts = {
	size, 0, flags
    };

    err = pcilib_stream_dma(ctx, dma, addr, size, flags, timeout, dma_ipe_skim_callback, &opts);
    if (read_bytes) *read_bytes = opts.pos;
    return err;
}


double dma_ipe_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    int err = 0;

    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

    int iter;
    size_t us = 0;
    struct timeval start, cur;
    
    void *buf;
    size_t bytes, rbytes;

    int (*read_dma)(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *read_bytes);

    if ((direction == PCILIB_DMA_TO_DEVICE)||(direction == PCILIB_DMA_BIDIRECTIONAL)) return -1.;

    if ((dma != PCILIB_DMA_ENGINE_INVALID)&&(dma > 1)) return -1.;

    if (size%IPEDMA_PAGE_SIZE) size = (1 + size / IPEDMA_PAGE_SIZE) * IPEDMA_PAGE_SIZE;

    err = dma_ipe_start(vctx, 0, PCILIB_DMA_FLAGS_DEFAULT);
    if (err) return err;

    if (getenv("PCILIB_BENCHMARK_HARDWARE"))
	read_dma = dma_ipe_skim_dma_custom;
    else
	read_dma = pcilib_read_dma_custom;

	// There is no significant difference and we can remove this when testing phase is over.
    if (getenv("PCILIB_BENCHMARK_STREAMING")) {
	size_t dma_buffer_space;
	pcilib_dma_engine_status_t dma_status;
	
	if (read_dma == pcilib_read_dma_custom)
	    pcilib_info_once("Benchmarking the DMA streaming (with memcpy)");
	else
	    pcilib_info_once("Benchmarking the DMA streaming (without memcpy)");

	    // Starting DMA
	WR(IPEDMA_REG_CONTROL, 0x1);

	gettimeofday(&start, NULL);
	pcilib_calc_deadline(&start, IPEDMA_DMA_TIMEOUT * IPEDMA_DMA_PAGES);

#ifdef IPEDMA_BUG_LAST_READ
	dma_buffer_space = (IPEDMA_DMA_PAGES - 2) * IPEDMA_PAGE_SIZE;
#else /* IPEDMA_BUG_LAST_READ */
	dma_buffer_space = (IPEDMA_DMA_PAGES - 1) * IPEDMA_PAGE_SIZE;
#endif /* IPEDMA_BUG_LAST_READ */

	// Allocate memory and prepare data
	buf = malloc(size + dma_buffer_space);
	if (!buf) return -1;

	    // Wait all DMA buffers are filled 
	memset(&dma_status, 0, sizeof(dma_status));
	do {
	    usleep(10 * IPEDMA_NODATA_SLEEP);
	    err = dma_ipe_get_status(vctx, dma, &dma_status, 0, NULL);
	} while ((!err)&&(dma_status.written_bytes < dma_buffer_space)&&(pcilib_calc_time_to_deadline(&start) > 0));

	if (err) {
	    pcilib_error("Error (%i) getting dma status", err);
	    return -1;
	} else if (dma_status.written_bytes < dma_buffer_space) {
	    pcilib_error("Timeout while waiting DMA engine to feel the buffer space completely, only %zu bytes of %zu written", dma_status.written_bytes, dma_buffer_space);
	    return -1;
	}

	gettimeofday(&start, NULL);
	for (iter = 0; iter < iterations; iter++) {
	    for (bytes = 0; bytes < (size + dma_buffer_space); bytes += rbytes) {
		err = read_dma(ctx->dmactx.pcilib, 0, addr, size + dma_buffer_space - bytes, PCILIB_DMA_FLAG_MULTIPACKET, PCILIB_DMA_TIMEOUT,  buf + bytes, &rbytes);
	        if (err) {
		    pcilib_error("Can't read data from DMA, error %i", err);
		    return -1;
		}
	    }
	    dma_buffer_space = 0;
	}

        gettimeofday(&cur, NULL);
	us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));

	    // Stopping DMA
	WR(IPEDMA_REG_CONTROL, 0x0);
	pcilib_skip_dma(ctx->dmactx.pcilib, 0);
    } else {
	if (read_dma == dma_ipe_skim_dma_custom)
	    pcilib_info_once("Benchmarking the DMA hardware (without memcpy)");

	WR(IPEDMA_REG_CONTROL, 0x0);
/*
	err = pcilib_skip_dma(ctx->dmactx.pcilib, 0);
	if (err) {
	    pcilib_error("Can't start benchmark, devices continuously writes unexpected data using DMA engine");
	    return -1;
	}
*/
	// Allocate memory and prepare data
	buf = malloc(size);
	if (!buf) return -1;

	for (iter = 0; iter < iterations; iter++) {
	    gettimeofday(&start, NULL);

	    // Starting DMA
	    WR(IPEDMA_REG_CONTROL, 0x1);
	
	    for (bytes = 0; bytes < size; bytes += rbytes) {
		err = read_dma(ctx->dmactx.pcilib, 0, addr, size - bytes, PCILIB_DMA_FLAG_MULTIPACKET, PCILIB_DMA_TIMEOUT,  buf + bytes, &rbytes);
	        if (err) {
		    pcilib_error("Can't read data from DMA (iteration: %zu, offset: %zu), error %i", iter, bytes, err);
		    return -1;
		}
	    }

		// Stopping DMA
	    WR(IPEDMA_REG_CONTROL, 0x0);
	    if (err) break;
	
	    gettimeofday(&cur, NULL);
	    us += ((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec));
/*	    
	    err = pcilib_skip_dma(ctx->dmactx.pcilib, 0);
	    if (err) {
		pcilib_error("Can't start iteration, devices continuously writes unexpected data using DMA engine");
		break;
	    }
*/
	}
    }

    free(buf);

    return err?-1:((1. * size * iterations * 1000000) / (1024. * 1024. * us));
}
