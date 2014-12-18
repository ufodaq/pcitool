#ifndef _PCILIB_DMA_NWL_H
#define _PCILIB_DMA_NWL_H

#include <stdio.h>
#include "../pcilib.h"

#define PCILIB_NWL_MODIFICATION_IPECAMERA 0x100

pcilib_dma_context_t *dma_nwl_init(pcilib_t *ctx, pcilib_dma_modification_t type, void *arg);
void  dma_nwl_free(pcilib_dma_context_t *vctx);

int dma_nwl_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);

int dma_nwl_enable_irq(pcilib_dma_context_t *vctx, pcilib_irq_type_t type, pcilib_dma_flags_t flags);
int dma_nwl_disable_irq(pcilib_dma_context_t *vctx, pcilib_dma_flags_t flags);
int dma_nwl_acknowledge_irq(pcilib_dma_context_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source);

int dma_nwl_start(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
int dma_nwl_stop(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

int dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *data, size_t *written);
int dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);


#ifdef _PCILIB_DMA_NWL_C
pcilib_dma_api_description_t nwl_dma_api = {
    "nwl_dma",
    dma_nwl_init,
    dma_nwl_free,
    dma_nwl_get_status,
    dma_nwl_enable_irq,
    dma_nwl_disable_irq,
    dma_nwl_acknowledge_irq,
    dma_nwl_start,
    dma_nwl_stop,
    dma_nwl_write_fragment,
    dma_nwl_stream_read,
    dma_nwl_benchmark
};
#else
extern pcilib_dma_api_description_t nwl_dma_api;
#endif


#endif /* _PCILIB_DMA_NWL_H */
