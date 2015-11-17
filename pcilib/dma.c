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
#include "tools.h"
#include "pagecpy.h"

const pcilib_dma_description_t *pcilib_get_dma_description(pcilib_t *ctx) {
    int err;

    err = pcilib_init_dma(ctx);
    if (err) {
	pcilib_error("Error (%i) while initializing DMA", err);
	return NULL;
    }

    if (!ctx->dma_ctx) return NULL;

    return &ctx->dma;
}


pcilib_dma_engine_t pcilib_find_dma_by_addr(pcilib_t *ctx, pcilib_dma_direction_t direction, pcilib_dma_engine_addr_t dma) {
    pcilib_dma_engine_t i;

    const pcilib_dma_description_t *dma_info =  pcilib_get_dma_description(ctx);
    if (!dma_info) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    for (i = 0; dma_info->engines[i].addr_bits; i++) {
	if ((dma_info->engines[i].addr == dma)&&((dma_info->engines[i].direction&direction)==direction)) break;
    }

    if (dma_info->engines[i].addr_bits) return i;
    return PCILIB_DMA_ENGINE_INVALID;
}


pcilib_dma_engine_t pcilib_add_dma_engine(pcilib_t *ctx, pcilib_dma_engine_description_t *desc) {
    pcilib_dma_engine_t engine = ctx->num_engines++;
    memcpy (&ctx->engines[engine], desc, sizeof(pcilib_dma_engine_description_t));
    return engine;
}


int pcilib_init_dma(pcilib_t *ctx) {
    int err;
    pcilib_dma_context_t *dma_ctx = NULL;
    const pcilib_dma_description_t *info = &ctx->dma;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    pcilib_dma_engine_t dma;

    if (ctx->dma_ctx)
	return 0;
	

    if ((ctx->event_ctx)&&(model_info->api)&&(model_info->api->init_dma)) {
	err = pcilib_init_register_banks(ctx);
	if (err) {
	    pcilib_error("Error (%i) while initializing register banks", err);
	    return err;
	}

	dma_ctx = model_info->api->init_dma(ctx->event_ctx);
    } else if ((ctx->dma.api)&&(ctx->dma.api->init)) {
	err = pcilib_init_register_banks(ctx);
	if (err) {
	    pcilib_error("Error (%i) while initializing register banks", err);
	    return err;
	}

	dma_ctx = ctx->dma.api->init(ctx, (ctx->dma.model?ctx->dma.model:ctx->model), ctx->dma.args);
    }

    if (dma_ctx) {
	for  (dma = 0; info->engines[dma].addr_bits; dma++) {
	    if (info->engines[dma].direction&PCILIB_DMA_FROM_DEVICE) {
		ctx->dma_rlock[dma] = pcilib_get_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, "dma%ir/%s", info->engines[dma].addr, info->name);
		if (!ctx->dma_rlock[dma]) break;
	    }
	    if (info->engines[dma].direction&PCILIB_DMA_TO_DEVICE) {
		ctx->dma_wlock[dma] = pcilib_get_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, "dma%iw/%s", info->engines[dma].addr, info->name);
		if (!ctx->dma_wlock[dma]) break;
	    }
	}

	if (info->engines[dma].addr_bits) {
	    if (ctx->dma.api->free)
		ctx->dma.api->free(dma_ctx);
	    pcilib_error("Failed to intialize DMA locks");
	    return PCILIB_ERROR_FAILED;
	}

	dma_ctx->pcilib = ctx;
	    // DS: parameters?
	ctx->dma_ctx = dma_ctx;
    }

    return 0;
}

int pcilib_start_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    if (!info->api->start_dma) {
	return 0;
    }

    return info->api->start_dma(ctx->dma_ctx, dma, flags);
}

int pcilib_stop_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);

    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!info->api->stop_dma) {
	return 0;
    }

    return info->api->stop_dma(ctx->dma_ctx, dma, flags);
}

int pcilib_enable_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);

    if ((!info)||(!info->api)||(!info->api->enable_irq)) return 0;

    return info->api->enable_irq(ctx->dma_ctx, irq_type, flags);
}

int pcilib_disable_irq(pcilib_t *ctx, pcilib_dma_flags_t flags) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);

    if ((!info)||(!info->api)||(!info->api->disable_irq)) return 0;

    return info->api->disable_irq(ctx->dma_ctx, flags);
}

int pcilib_acknowledge_irq(pcilib_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);

    if ((!info)||(!info->api)||(!info->api->acknowledge_irq)) return 0;

    return info->api->acknowledge_irq(ctx->dma_ctx, irq_type, irq_source);
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

    pcilib_pagecpy(ctx->data + ctx->pos, buf, bufsize);
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
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!info->api->stream) {
	pcilib_error("The DMA read is not supported by configured DMA engine");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

	// DS: We should check we are not going outside of allocated engine space
    if (!info->engines[dma].addr_bits) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    if ((info->engines[dma].direction&PCILIB_DMA_FROM_DEVICE) == 0) {
	pcilib_error("The selected engine (%i) is S2C-only and does not support reading", dma);
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    err = pcilib_try_lock(ctx->dma_rlock[dma]);
    if (err) {
	if ((err == PCILIB_ERROR_BUSY)||(err == PCILIB_ERROR_TIMEOUT))
	    pcilib_error("DMA engine (%i) is busy", dma);
	else
	    pcilib_error("Error (%i) locking DMA engine (%i)", err, dma);

	return err;
    }

    err = info->api->stream(ctx->dma_ctx, dma, addr, size, flags, timeout, cb, cbattr);

    pcilib_unlock(ctx->dma_rlock[dma]);

    return err;
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

    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return PCILIB_ERROR_NOTAVAILABLE;
    }
    
    if (!info->api->push) {
	pcilib_error("The DMA write is not supported by configured DMA engine");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
	    // DS: We should check we don't exceed allocated engine range
    if (!info->engines[dma].addr_bits) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    if ((info->engines[dma].direction&PCILIB_DMA_TO_DEVICE) == 0) {
	pcilib_error("The selected engine (%i) is C2S-only and does not support writes", dma);
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    err = pcilib_try_lock(ctx->dma_wlock[dma]);
    if (err) {
	if (err == PCILIB_ERROR_BUSY) 
	    pcilib_error("DMA engine (%i) is busy", dma);
	else
	    pcilib_error("Error (%i) locking DMA engine (%i)", err, dma);

	return err;
    }

    err = info->api->push(ctx->dma_ctx, dma, addr, size, flags, timeout, buf, written);

    pcilib_unlock(ctx->dma_wlock[dma]);

    return err;
}


int pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf, size_t *written_bytes) {
    return pcilib_push_dma(ctx, dma, addr, size, PCILIB_DMA_FLAG_EOP|PCILIB_DMA_FLAG_WAIT, PCILIB_DMA_TIMEOUT, buf, written_bytes);
}

double pcilib_benchmark_dma(pcilib_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return 0;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return -1;
    }
    
    if (!info->api->benchmark) {
	pcilib_error("The DMA benchmark is not supported by configured DMA engine");
	return -1;
    }

    return info->api->benchmark(ctx->dma_ctx, dma, addr, size, iterations, direction);
}

int pcilib_get_dma_status(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers) {
    const pcilib_dma_description_t *info =  pcilib_get_dma_description(ctx);
    if (!info) {
	pcilib_error("DMA is not supported by the device");
	return 0;
    }

    if (!info->api) {
	pcilib_error("DMA Engine is not configured in the current model");
	return -1;
    }
    
    if (!info->api->status) {
	memset(status, 0, sizeof(pcilib_dma_engine_status_t));
	return -1;
   }
    
        // DS: We should check we don't exceed allocated engine range
    if (!info->engines[dma].addr_bits) {
	pcilib_error("The DMA engine (%i) is not supported by device", dma);
	return -1;
    }

    return info->api->status(ctx->dma_ctx, dma, status, n_buffers, buffers);
}
