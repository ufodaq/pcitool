#ifndef _PCILIB_DMA_NWL_ENGINE_H
#define _PCILIB_DMA_NWL_ENGINE_H

int dma_nwl_read_engine_config(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, char *base);
int dma_nwl_start_engine(nwl_dma_t *ctx, pcilib_dma_engine_t dma);
int dma_nwl_stop_engine(nwl_dma_t *ctx, pcilib_dma_engine_t dma);

int dma_nwl_wait_completion(nwl_dma_t * ctx, pcilib_dma_engine_t dma, pcilib_timeout_t timeout);


#endif /* _PCILIB_DMA_NWL_ENGINE_H */

