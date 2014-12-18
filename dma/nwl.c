#define _PCILIB_DMA_NWL_C
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

int dma_nwl_start(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    int err;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    if (!ctx->started) {
	// global initialization, should we do anything?
	ctx->started = 1;
    }

    if (dma == PCILIB_DMA_ENGINE_INVALID) return 0;
    else if (dma > ctx->n_engines) return PCILIB_ERROR_INVALID_BANK;

    if (flags&PCILIB_DMA_FLAG_PERSISTENT) ctx->engines[dma].preserve = 1;

    err = dma_nwl_start_engine(ctx, dma);
    
    return err;
}

int dma_nwl_stop(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    int err;
    int preserving = 0;

    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    
    if (!ctx->started) return 0;
    
	// stop everything
    if (dma == PCILIB_DMA_ENGINE_INVALID) {
        for (dma = 0; dma < ctx->n_engines; dma++) {
	    if (flags&PCILIB_DMA_FLAG_PERSISTENT) {
		ctx->engines[dma].preserve = 0;
	    }
	
	    if (ctx->engines[dma].preserve) preserving = 1;
	    
	    err = dma_nwl_stop_engine(ctx, dma);
	    if (err) return err;
	}
	    
	    // global cleanup, should we do anything?
	if (!preserving) {
	    ctx->started = 0;
	}
	
	return 0;
    }
    
    if (dma > ctx->n_engines) return PCILIB_ERROR_INVALID_BANK;
    
	    // ignorign previous setting if flag specified
    if (flags&PCILIB_DMA_FLAG_PERSISTENT) {
	ctx->engines[dma].preserve = 0;
    }
    
    return dma_nwl_stop_engine(ctx, dma);
}


pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib, pcilib_dma_modification_t type, void *arg) {
    int i;
    int err;
    pcilib_dma_engine_t n_engines;

    pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (ctx) {
	memset(ctx, 0, sizeof(nwl_dma_t));
	ctx->pcilib = pcilib;
	ctx->type = type;

	if (type == PCILIB_NWL_MODIFICATION_IPECAMERA) {
	    ctx->dmactx.ignore_eop = 1;
	}

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
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    if (ctx) {
	if (ctx->type == PCILIB_DMA_MODIFICATION_DEFAULT) dma_nwl_stop_loopback(ctx);
	dma_nwl_free_irq(ctx);
	dma_nwl_stop(vctx, PCILIB_DMA_ENGINE_ALL, PCILIB_DMA_FLAGS_DEFAULT);
	    
	free(ctx);
    }
}
