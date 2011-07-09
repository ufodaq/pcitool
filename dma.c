#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "error.h"
#include "pcilib.h"
#include "pci.h"
#include "dma.h"

const pcilib_dma_info_t *pcilib_get_dma_info(pcilib_t *ctx) {
    if (!ctx->dma_ctx) {
	pcilib_model_t model = pcilib_get_model(ctx);
	pcilib_dma_api_description_t *api = pcilib_model[model].dma_api;
	
	if ((api)&&(api->init)) {
	    pcilib_map_register_space(ctx);
	    ctx->dma_ctx = api->init(ctx);
	}
	
	if (!ctx->dma_ctx) return NULL;
    }
    
    return &ctx->dma_info;
}

pcilib_dma_engine_t pcilib_find_dma_by_addr(pcilib_t *ctx, pcilib_dma_direction_t direction, pcilib_dma_engine_addr_t dma) {
    pcilib_dma_engine_t i;

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    for (i = 0; info->engines[i]; i++) {
	if ((info->engines[i]->addr == dma)&&((info->engines[i]->direction&direction)==direction)) break;
    }
    
    if (info->engines[i]) return i;
    return PCILIB_DMA_INVALID;
}

int pcilib_set_dma_engine_description(pcilib_t *ctx, pcilib_dma_engine_t engine, pcilib_dma_engine_description_t *desc) {
    ctx->dma_info.engines[engine] = desc;
}

typedef struct {
    size_t size;
    void *data;
    size_t pos;
} pcilib_dma_read_callback_context_t;

static int pcilib_dma_read_callback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    pcilib_dma_read_callback_context_t *ctx = (pcilib_dma_read_callback_context_t*)arg;
    
    if (ctx->pos + bufsize > ctx->size) {
	pcilib_error("Buffer size (%li) is not large enough for DMA packet, at least %li bytes is required", ctx->size, ctx->pos + bufsize); 
	return PCILIB_ERROR_INVALID_DATA;
    }
    
    memcpy(ctx->data + ctx->pos, buf, bufsize);
    ctx->pos += bufsize;

    if (flags & PCILIB_DMA_FLAG_EOP) return 0;
    return 1;
}

static int pcilib_dma_skip_callback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    return 1;
}

size_t pcilib_stream_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return 0;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return 0;
    }
    
    if (!ctx->model_info.dma_api->stream) {
	pcilib_error("The DMA read is not supported by configured DMA engine");
	return 0;
    }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return 0;
    }

    if (info->engines[dma]->direction&PCILIB_DMA_FROM_DEVICE == 0) {
	pcilib_error("The selected engine (%i) is S2C-only and does not support reading", dma);
	return 0;
    }

    return ctx->model_info.dma_api->stream(ctx->dma_ctx, dma, addr, size, flags, timeout, cb, cbattr);
}

size_t pcilib_read_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf) {
    int err; 

    pcilib_dma_read_callback_context_t opts = {
	size, buf, 0
    };

    return pcilib_stream_dma(ctx, dma, addr, size, PCILIB_DMA_FLAGS_DEFAULT, PCILIB_DMA_TIMEOUT, pcilib_dma_read_callback, &opts);
}

int pcilib_skip_dma(pcilib_t *ctx, pcilib_dma_engine_t dma) {
    size_t skipped;
    do {
	    // IMMEDIATE timeout is not working properly, so default is set
	skipped = pcilib_stream_dma(ctx, dma, 0, 0, PCILIB_DMA_FLAGS_DEFAULT, PCILIB_DMA_TIMEOUT, pcilib_dma_skip_callback, NULL);
    } while (skipped > 0);
    
    return 0;
}


size_t pcilib_push_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, void *buf) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return 0;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return 0;
    }
    
    if (!ctx->model_info.dma_api->push) {
	pcilib_error("The DMA write is not supported by configured DMA engine");
	return 0;
    }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return 0;
    }

    if (info->engines[dma]->direction&PCILIB_DMA_TO_DEVICE == 0) {
	pcilib_error("The selected engine (%i) is C2S-only and does not support writes", dma);
	return 0;
    }
    
    return ctx->model_info.dma_api->push(ctx->dma_ctx, dma, addr, size, flags, timeout, buf);
}


size_t pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf) {
    return pcilib_push_dma(ctx, dma, addr, size, PCILIB_DMA_FLAG_EOP, PCILIB_DMA_TIMEOUT, buf);
}

double pcilib_benchmark_dma(pcilib_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return 0;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return -1;
    }
    
    if (!ctx->model_info.dma_api->benchmark) {
	pcilib_error("The DMA benchmark is not supported by configured DMA engine");
	return -1;
   }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return -1;
    }

    return ctx->model_info.dma_api->benchmark(ctx->dma_ctx, dma, addr, size, iterations, direction);
}
