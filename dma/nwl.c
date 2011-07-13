#define _PCILIB_DMA_NWL_C
#define _BSD_SOURCE
//#define DEBUG_HARDWARE

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

int dma_nwl_start(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    if (!ctx->started) {
	// global initialization, should we do anything?
	ctx->started = 1;
    }

    if (dma == PCILIB_DMA_ENGINE_INVALID) return 0;
    else if (dma > ctx->n_engines) return PCILIB_ERROR_INVALID_BANK;

    if (flags&PCILIB_DMA_FLAG_PERMANENT) ctx->engines[dma].preserve = 1;
    return dma_nwl_start_engine(ctx, dma);
}

int dma_nwl_stop(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    int err;
    int preserving = 0;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    
    if (!ctx->started) return 0;
    
	// stop everything
    if (dma == PCILIB_DMA_ENGINE_INVALID) {
        for (dma = 0; dma < ctx->n_engines; dma++) {
	    if (flags&PCILIB_DMA_FLAG_PERMANENT) {
		ctx->engines[dma].preserve = 0;
	    }
	
	    if (ctx->engines[dma].preserve) preserving = 1;
	    else {
	        err = dma_nwl_stop_engine(ctx, dma);
		if (err) return err;
	    }
	}
	    
	    // global cleanup, should we do anything?
	if (!preserving) {
	    ctx->started = 0;
	}
	
	return 0;
    }
    
    if (dma > ctx->n_engines) return PCILIB_ERROR_INVALID_BANK;
    
	    // ignorign previous setting if flag specified
    if (flags&PCILIB_DMA_FLAG_PERMANENT) {
	ctx->engines[dma].preserve = 0;
    }
    
	// Do not close DMA if preservation mode is set
    if (ctx->engines[dma].preserve) return 0;
    
    return dma_nwl_stop_engine(ctx, dma);
}


pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib, pcilib_dma_modification_t type, void *arg) {
    int i;
    int err;
    uint32_t val;
    pcilib_dma_engine_t n_engines;

    pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (ctx) {
	memset(ctx, 0, sizeof(nwl_dma_t));
	ctx->pcilib = pcilib;
	ctx->type = type;

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
    int err;
    
    pcilib_dma_engine_t i;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    if (ctx) {
	if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) dma_nwl_stop_loopback(ctx);
	dma_nwl_free_irq(ctx);
	dma_nwl_stop(ctx, PCILIB_DMA_ENGINE_ALL, PCILIB_DMA_FLAGS_DEFAULT);
	    
	free(ctx);
    }
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
