#ifndef _PCILIB_DMA_NWL_LOOPBACK_H
#define _PCILIB_DMA_NWL_LOOPBACK_H

int dma_nwl_start_loopback(nwl_dma_t *ctx,  pcilib_dma_direction_t direction, size_t packet_size);
int dma_nwl_stop_loopback(nwl_dma_t *ctx);

#endif /* _PCILIB_DMA_NWL_LOOPBACK_H */
