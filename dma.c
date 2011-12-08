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
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "error.h"
#include "pcilib.h"
#include "pci.h"
#include "dma.h"

const pcilib_dma_info_t *pcilib_get_dma_info(pcilib_t *ctx) {
    if (!ctx->dma_ctx) {
        pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

	if ((ctx->event_ctx)&&(model_info->event_api->init_dma)) {
	    pcilib_map_register_space(ctx);
	    ctx->dma_ctx = model_info->event_api->init_dma(ctx->event_ctx);
	} else if ((model_info->dma_api)&&(model_info->dma_api->init)) {
	    pcilib_map_register_space(ctx);
	    ctx->dma_ctx = model_info->dma_api->init(ctx, PCILIB_DMA_MODIFICATION_DEFAULT, NULL);
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
    return PCILIB_DMA_ENGINE_INVALID;
}

int pcilib_set_dma_engine_description(pcilib_t *ctx, pcilib_dma_engine_t engine, pcilib_dma_engine_description_t *desc) {
    ctx->dma_info.engines[engine] = desc;
}

int pcilib_start_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->start_dma) {
	return 0;
    }
    
    return ctx->model_info.dma_api->start_dma(ctx->dma_ctx, dma, flags);
}

int pcilib_stop_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->stop_dma) {
	return 0;
    }

    return ctx->model_info.dma_api->stop_dma(ctx->dma_ctx, dma, flags);
}

int pcilib_enable_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->enable_irq) {
	return 0;
    }

    return ctx->model_info.dma_api->enable_irq(ctx->dma_ctx, irq_type, flags);
}

int pcilib_disable_irq(pcilib_t *ctx, pcilib_dma_flags_t flags) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->disable_irq) {
	return 0;
    }
    
    return ctx->model_info.dma_api->disable_irq(ctx->dma_ctx, flags);
}

int pcilib_acknowledge_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->acknowledge_irq) {
	return 0;
    }

    return ctx->model_info.dma_api->acknowledge_irq(ctx->dma_ctx, irq_type, irq_source);
}



typedef struct {
    size_t size;
    void *data;
    size_t pos;
    
    pcilib_dma_flags_t flags;
} pcilib_dma_read_callback_context_t;

static int pcilib_dma_read_callback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    pcilib_dma_read_callback_context_t *ctx = (pcilib_dma_read_callback_context_t*)arg;
    
    if (ctx->pos + bufsize > ctx->size) {
	if ((ctx->flags&PCILIB_DMA_FLAG_IGNORE_ERRORS) == 0)
	    pcilib_error("Buffer size (%li) is not large enough for DMA packet, at least %li bytes is required", ctx->size, ctx->pos + bufsize); 
	return -PCILIB_ERROR_TOOBIG;
    }
    
    memcpy(ctx->data + ctx->pos, buf, bufsize);
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

static int pcilib_dma_skip_callback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    struct timeval *tv = (struct timeval*)arg;
    struct timeval cur;
    
    if (tv) {
	gettimeofday(&cur, NULL);
	if ((cur.tv_sec > tv->tv_sec)||((cur.tv_sec == tv->tv_sec)&&(cur.tv_usec > tv->tv_usec))) return PCILIB_STREAMING_STOP;
    }
    
    return PCILIB_STREAMING_REQ_PACKET;
}

int pcilib_stream_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->stream) {
	pcilib_error("The DMA read is not supported by configured DMA engine");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    if (info->engines[dma]->direction&PCILIB_DMA_FROM_DEVICE == 0) {
	pcilib_error("The selected engine (%i) is S2C-only and does not support reading", dma);
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    return ctx->model_info.dma_api->stream(ctx->dma_ctx, dma, addr, size, flags, timeout, cb, cbattr);
}

int pcilib_read_dma_custom(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *read_bytes) {
    int err; 

    pcilib_dma_read_callback_context_t opts = {
	size, buf, 0, flags
    };
    
    err = pcilib_stream_dma(ctx, dma, addr, size, flags, timeout, pcilib_dma_read_callback, &opts);
    if (read_bytes) *read_bytes = opts.pos;
    return err;
}

int pcilib_read_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *read_bytes) {
    int err; 

    pcilib_dma_read_callback_context_t opts = {
	size, buf, 0, 0
    };
    
    err = pcilib_stream_dma(ctx, dma, addr, size, PCILIB_DMA_FLAGS_DEFAULT, PCILIB_DMA_TIMEOUT, pcilib_dma_read_callback, &opts);
    if (read_bytes) *read_bytes = opts.pos;
    return err;
}


int pcilib_skip_dma(pcilib_t *ctx, pcilib_dma_engine_t dma) {
    int err;
    struct timeval tv, cur;

    gettimeofday(&tv, NULL);
    tv.tv_usec += PCILIB_DMA_SKIP_TIMEOUT;
    tv.tv_sec += tv.tv_usec / 1000000;
    tv.tv_usec += tv.tv_usec % 1000000;
    
    do {
	    // IMMEDIATE timeout is not working properly, so default is set
	err = pcilib_stream_dma(ctx, dma, 0, 0, PCILIB_DMA_FLAGS_DEFAULT, PCILIB_DMA_TIMEOUT, pcilib_dma_skip_callback, &tv);
	gettimeofday(&cur, NULL);
    } while ((!err)&&((cur.tv_sec < tv.tv_sec)||((cur.tv_sec == tv.tv_sec)&&(cur.tv_usec < tv.tv_usec))));

    if ((cur.tv_sec > tv.tv_sec)||((cur.tv_sec == tv.tv_sec)&&(cur.tv_usec > tv.tv_usec))) return PCILIB_ERROR_TIMEOUT;
    
    return 0;
}


int pcilib_push_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *written) {
    int err; 

    const pcilib_dma_info_t *info =  pcilib_get_dma_info(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!ctx->model_info.dma_api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!ctx->model_info.dma_api->push) {
	pcilib_error("The DMA write is not supported by configured DMA engine");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    if (info->engines[dma]->direction&PCILIB_DMA_TO_DEVICE == 0) {
	pcilib_error("The selected engine (%i) is C2S-only and does not support writes", dma);
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    return ctx->model_info.dma_api->push(ctx->dma_ctx, dma, addr, size, flags, timeout, buf, written);
}


int pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *written_bytes) {
    return pcilib_push_dma(ctx, dma, addr, size, PCILIB_DMA_FLAG_EOP|PCILIB_DMA_FLAG_WAIT, PCILIB_DMA_TIMEOUT, buf, written_bytes);
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

int pcilib_get_dma_status(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers) {
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
    
    if (!ctx->model_info.dma_api->status) {
	memset(status, 0, sizeof(pcilib_dma_engine_status_t));
	return -1;
   }
    
    if (!info->engines[dma]) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return -1;
    }

    return ctx->model_info.dma_api->status(ctx->dma_ctx, dma, status, n_buffers, buffers);

}
