#ifndef _PCILIB_DMA_IPE_H
#define _PCILIB_DMA_IPE_H

#include <stdio.h>
#include "../pcilib.h"

//#define PCILIB_NWL_MODIFICATION_IPECAMERA 0x100

pcilib_dma_context_t *dma_ipe_init(pcilib_t *ctx, pcilib_dma_modification_t type, void *arg);
void  dma_ipe_free(pcilib_dma_context_t *vctx);

int dma_ipe_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);


int dma_ipe_start(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
int dma_ipe_stop(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

int dma_ipe_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
double dma_ipe_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);


#ifdef _PCILIB_DMA_IPE_C
pcilib_dma_api_description_t ipe_dma_api = {
    "ipe_dma",
    dma_ipe_init,
    dma_ipe_free,
    dma_ipe_get_status,
    NULL,
    NULL,
    NULL,
    dma_ipe_start,
    dma_ipe_stop,
    NULL,
    dma_ipe_stream_read,
    dma_ipe_benchmark
};
#else
extern pcilib_dma_api_description_t ipe_dma_api;
#endif


#endif /* _PCILIB_DMA_IPE_H */
