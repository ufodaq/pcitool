#define _PCILIB_DMA_NWL_C
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

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
    else if (dma > ctx->dmactx.pcilib->num_engines) return PCILIB_ERROR_INVALID_BANK;

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
        for (dma = 0; dma < ctx->dmactx.pcilib->num_engines; dma++) {
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
    
    if (dma > ctx->dmactx.pcilib->num_engines) return PCILIB_ERROR_INVALID_BANK;
    
	    // ignorign previous setting if flag specified
    if (flags&PCILIB_DMA_FLAG_PERSISTENT) {
	ctx->engines[dma].preserve = 0;
    }
    
    return dma_nwl_stop_engine(ctx, dma);
}


pcilib_dma_context_t *dma_nwl_init(pcilib_t *pcilib, const char *model, const void *arg) {
    int i, j;
    int err;
    
    pcilib_dma_engine_t dma;
    pcilib_register_description_t eregs[NWL_MAX_DMA_ENGINE_REGISTERS];
    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
    
    nwl_dma_t *ctx = malloc(sizeof(nwl_dma_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(nwl_dma_t));

    pcilib_register_bank_t dma_bank = pcilib_find_register_bank_by_addr(pcilib, PCILIB_REGISTER_BANK_DMA);
    if (dma_bank == PCILIB_REGISTER_BANK_INVALID) {
	free(ctx);
	pcilib_error("DMA Register Bank could not be found");
	return NULL;
    }

    ctx->dma_bank = model_info->banks + dma_bank;
    ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);

    if ((model)&&(strcasestr(model, "ipecamera"))) {
	ctx->type = NWL_MODIFICATION_IPECAMERA;
	ctx->ignore_eop = 1;
    } else {
	ctx->type = NWL_MODIFICATION_DEFAULT;
	    
	err = pcilib_add_registers(pcilib, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, 0, nwl_xrawdata_registers, NULL);
	if (err) {
	    free(ctx);
	    pcilib_error("Error (%i) adding NWL XRAWDATA registers", err);
	    return NULL;
	}
    }

    for (j = 0; nwl_dma_engine_registers[j].bits; j++);
    if (j > NWL_MAX_DMA_ENGINE_REGISTERS) {
	free(ctx);
	pcilib_error("Too many (%i) engine registers defined, only %i supported", j, NWL_MAX_DMA_ENGINE_REGISTERS);
	return NULL;
    }

    memcpy(eregs, nwl_dma_engine_registers, j * sizeof(pcilib_register_description_t));

    for (i = 0; i < 2 * PCILIB_MAX_DMA_ENGINES; i++) {
	pcilib_dma_engine_description_t edesc;
	char *addr = ctx->base_addr + DMA_OFFSET + i * DMA_ENGINE_PER_SIZE;

	memset(&edesc, 0, sizeof(pcilib_dma_engine_description_t));

	err = dma_nwl_read_engine_config(ctx, &edesc, addr);
	if (err) continue;

	for (j = 0; nwl_dma_engine_registers[j].bits; j++) {
	    int dma_addr_len = (PCILIB_MAX_DMA_ENGINES > 9)?2:1;
	    const char *dma_direction;

	    eregs[j].name = nwl_dma_engine_register_names[i * NWL_MAX_DMA_ENGINE_REGISTERS + j];
	    eregs[j].addr = nwl_dma_engine_registers[j].addr + (addr - ctx->base_addr);
	    
	    switch (edesc.direction) {
		case PCILIB_DMA_FROM_DEVICE:
		    dma_direction =  "r";
		break;
		case PCILIB_DMA_TO_DEVICE:
		    dma_direction = "w";
		break;
		default:
		    dma_direction = "";
	    }    
	    sprintf((char*)eregs[j].name, nwl_dma_engine_registers[j].name, dma_addr_len, edesc.addr, dma_direction);
	}
	
        err = pcilib_add_registers(pcilib, PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT, j, eregs, NULL);
	if (err) {
	    free(ctx);
	    pcilib_error("Error (%i) adding NWL DMA registers for engine %i", err, edesc.addr);
	    return NULL;
	}
    
	dma = pcilib_add_dma_engine(pcilib, &edesc);
	if (dma == PCILIB_DMA_ENGINE_INVALID) {
	    free(ctx);
	    pcilib_error("Problem while registering DMA engine");
	    return NULL;
	}
	
	ctx->engines[dma].desc = &pcilib->engines[dma];
	ctx->engines[dma].base_addr = addr;
    }

    return (pcilib_dma_context_t*)ctx;
}

void  dma_nwl_free(pcilib_dma_context_t *vctx) {
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;

    if (ctx) {
	if (ctx->type == NWL_MODIFICATION_DEFAULT) dma_nwl_stop_loopback(ctx);
	dma_nwl_free_irq(ctx);
	dma_nwl_stop(vctx, PCILIB_DMA_ENGINE_ALL, PCILIB_DMA_FLAGS_DEFAULT);
	    
	free(ctx);
    }
}
