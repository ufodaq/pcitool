#ifndef _PCILIB_NWL_IRQ_H
#define _PCILIB_NWL_IRQ_H

int dma_nwl_enable_irq(nwl_dma_t *ctx, pcilib_irq_type_t type);
int dma_nwl_disable_irq(nwl_dma_t *ctx);
int dma_nwl_enable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma);
int dma_nwl_disable_engine_irq(nwl_dma_t *ctx, pcilib_dma_engine_t dma);

#endif /* _PCILIB_NWL_IRQ_H */
