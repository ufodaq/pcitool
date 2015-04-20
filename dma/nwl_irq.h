#ifndef _PCILIB_DMA_NWL_IRQ_H
#define _PCILIB_DMA_NWL_IRQ_H

int dma_nwl_init_irq(nwl_dma_t *ctx, uint32_t val);
int dma_nwl_free_irq(nwl_dma_t *ctx);

int dma_nwl_enable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma);
int dma_nwl_disable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma);

#endif /* _PCILIB_DMA_NWL_IRQ_H */
