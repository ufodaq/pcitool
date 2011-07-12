#ifndef _PCILIB_DMA_H
#define _PCILIB_DMA_H

#define PCILIB_DMA_BUFFER_INVALID ((size_t)-1)

struct pcilib_dma_api_description_s {
    pcilib_dma_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_dma_context_t *ctx);

    int (*pcilib_start_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
    int (*pcilib_stop_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

    int (*pcilib_enable_irq)(pcilib_dma_context_t *ctx, pcilib_dma_flags_t flags);
    int (*pcilib_disable_irq)(pcilib_dma_context_t *ctx, pcilib_dma_flags_t flags);

    int (*push)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *written);
    int (*stream)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);

    double (*benchmark)(pcilib_dma_context_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);
};

int pcilib_set_dma_engine_description(pcilib_t *ctx, pcilib_dma_engine_t engine, pcilib_dma_engine_description_t *desc);

#endif /* _PCILIB_DMA_H */
