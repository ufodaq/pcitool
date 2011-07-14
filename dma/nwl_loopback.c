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


int dma_nwl_start_loopback(nwl_dma_t *ctx,  pcilib_dma_direction_t direction, size_t packet_size) {
    uint32_t val;

	// Re-initializing always
    
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

    ctx->loopback_started = 1;
        
    return 0;
}

int dma_nwl_stop_loopback(nwl_dma_t *ctx) {
    uint32_t val = 0;

	/* Stop in any case, otherwise we can have problems in benchmark due to
	engine initialized in previous run, and benchmark is only actual usage.
	Otherwise, we should detect current loopback status during initialization */
    nwl_write_register(val, ctx, ctx->base_addr, TX_CONFIG_ADDRESS);
    nwl_write_register(val, ctx, ctx->base_addr, RX_CONFIG_ADDRESS);
    ctx->loopback_started = 0;
    
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
    pcilib_register_value_t regval;

    size_t us = 0;
    struct timeval start, cur;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    pcilib_dma_engine_t readid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_FROM_DEVICE, dma);
    pcilib_dma_engine_t writeid = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_TO_DEVICE, dma);

    char *read_base = ctx->engines[readid].base_addr;
    char *write_base = ctx->engines[writeid].base_addr;

    if (size%sizeof(uint32_t)) size = 1 + size / sizeof(uint32_t);
    else size /= sizeof(uint32_t);

	// Not supported
    if (direction == PCILIB_DMA_TO_DEVICE) return -1.;
    else if ((direction == PCILIB_DMA_FROM_DEVICE)&&(ctx->type != PCILIB_DMA_MODIFICATION_DEFAULT)) return -1.;

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


    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) {
	dma_nwl_start_loopback(ctx, direction, size * sizeof(uint32_t));
    }

	// Allocate memory and prepare data
    buf = malloc(size * sizeof(uint32_t));
    cmp = malloc(size * sizeof(uint32_t));
    if ((!buf)||(!cmp)) {
	if (buf) free(buf);
	if (cmp) free(cmp);
	return -1;
    }

    memset(cmp, 0x13, size * sizeof(uint32_t));


#ifdef DEBUG_HARDWARE	     
    if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e5);
	usleep(100000);
	pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e1);
    }
#endif /* DEBUG_HARDWARE */

	// Benchmark
    for (i = 0; i < iterations; i++) {
#ifdef DEBUG_HARDWARE	     
	if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	    pcilib_write_register(ctx->pcilib, NULL, "control", 0x1e1);
	}
#endif /* DEBUG_HARDWARE */

        gettimeofday(&start, NULL);
	if (direction&PCILIB_DMA_TO_DEVICE) {
	    memcpy(buf, cmp, size * sizeof(uint32_t));

	    err = pcilib_write_dma(ctx->pcilib, writeid, addr, size * sizeof(uint32_t), buf, &bytes);
	    if ((err)||(bytes != size * sizeof(uint32_t))) {
		error = "Write failed";
	        break;
	    }
	}

#ifdef DEBUG_HARDWARE	     
	if (ctx->type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	    //usleep(1000000);
	    pcilib_write_register(ctx->pcilib, NULL, "control", 0x3e1);
	}

	memset(buf, 0, size * sizeof(uint32_t));
#endif /* DEBUG_HARDWARE */
        
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

#ifdef DEBUG_HARDWARE	     
	puts("====================================");

	err = pcilib_read_register(ctx->pcilib, NULL, "reg9050", &regval);
	printf("Status1: %i 0x%lx\n", err, regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9080", &regval);
	printf("Start address: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9090", &regval);
	printf("End address: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9100", &regval);
	printf("Status2: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9110", &regval);
	printf("Status3: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9160", &regval);
	printf("Add_rd_ddr: %i 0x%lx\n", err, regval);
#endif /* DEBUG_HARDWARE */

    }

#ifdef DEBUG_HARDWARE	     
    puts("------------------------------------------------");
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9050", &regval);
    printf("Status1: %i 0x%lx\n", err, regval);
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9080", &regval);
    printf("Start address: %i 0x%lx\n", err,  regval);
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9090", &regval);
    printf("End address: %i 0x%lx\n", err,  regval);
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9100", &regval);
    printf("Status2: %i 0x%lx\n", err,  regval);
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9110", &regval);
    printf("Status3: %i 0x%lx\n", err,  regval);
    err = pcilib_read_register(ctx->pcilib, NULL, "reg9160", &regval);
    printf("Add_rd_ddr: %i 0x%lx\n", err, regval);
#endif /* DEBUG_HARDWARE */

    if (error) {
	pcilib_warning("%s at iteration %i, error: %i, bytes: %zu", error, i, err, bytes);
    }
    
#ifdef NWL_GENERATE_DMA_IRQ
    dma_nwl_disable_engine_irq(ctx, writeid);
    dma_nwl_disable_engine_irq(ctx, readid);
#endif /* NWL_GENERATE_DMA_IRQ */

    if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) dma_nwl_stop_loopback(ctx);

    __sync_synchronize();
    
    if (direction == PCILIB_DMA_FROM_DEVICE) {
	pcilib_skip_dma(ctx->pcilib, readid);
    }
    
    free(cmp);
    free(buf);

    return error?-1:(1. * size * sizeof(uint32_t) * iterations * 1000000) / (1024. * 1024. * us);
}
