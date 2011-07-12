#ifndef _PCILIB_DMA_NWL_H
#define _PCILIB_DMA_NWL_H

#include <stdio.h>
#include "pcilib.h"

#define NWL_DMA_IRQ_SOURCE 0

typedef struct nwl_dma_s nwl_dma_t;

pcilib_dma_context_t *dma_nwl_init(pcilib_t *ctx);
void  dma_nwl_free(pcilib_dma_context_t *vctx);

int dma_nwl_enable_irq(pcilib_dma_context_t *vctx, pcilib_irq_type_t type, pcilib_dma_flags_t flags);
int dma_nwl_disable_irq(pcilib_dma_context_t *vctx, pcilib_dma_flags_t flags);

int dma_nwl_write_fragment(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *data, size_t *written);
int dma_nwl_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);
double dma_nwl_benchmark(pcilib_dma_context_t *vctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);


#ifdef _PCILIB_DMA_NWL_C
pcilib_dma_api_description_t nwl_dma_api = {
    dma_nwl_init,
    dma_nwl_free,
    dma_nwl_enable_irq,
    dma_nwl_disable_irq,
    NULL,
    NULL,
    dma_nwl_write_fragment,
    dma_nwl_stream_read,
    dma_nwl_benchmark
};
#else
extern pcilib_dma_api_description_t nwl_dma_api;
#endif


#endif /* _PCILIB_DMA_NWL_H */
