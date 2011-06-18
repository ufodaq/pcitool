#ifndef _PCILIB_DMA_NWL_H
#define _PCILIB_DMA_NWL_H

#include <stdio.h>
#include "pcilib.h"

typedef struct nwl_dma_s nwl_dma_t;

/*
typedef struct {
    pcilib_dma_engine_info_t info;
    // offset  
} pcilib_dma_engine_info_t;
*/


pcilib_dma_context_t *dma_nwl_init(pcilib_t *ctx);
void  dma_nwl_free(pcilib_dma_context_t *vctx);

int dma_nwl_read(pcilib_dma_context_t *ctx, pcilib_dma_t dma, size_t size, void *buf);

#ifdef _PCILIB_DMA_NWL_C
pcilib_dma_api_description_t nwl_dma_api = {
    dma_nwl_init,
    dma_nwl_free,
    dma_nwl_read
};
#else
extern pcilib_dma_api_description_t nwl_dma_api;
#endif


#endif /* _PCILIB_DMA_NWL_H */
