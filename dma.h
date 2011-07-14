#ifndef _PCILIB_DMA_H
#define _PCILIB_DMA_H

#define PCILIB_DMA_BUFFER_INVALID ((size_t)-1)
#define PCILIB_DMA_MODIFICATION_DEFAULT 0		/**< first 0x100 are reserved */

typedef uint32_t pcilib_dma_modification_t;


struct pcilib_dma_api_description_s {
    pcilib_dma_context_t *(*init)(pcilib_t *ctx, pcilib_dma_modification_t type, void *arg);
    void (*free)(pcilib_dma_context_t *ctx);

    int (*enable_irq)(pcilib_dma_context_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags);
    int (*disable_irq)(pcilib_dma_context_t *ctx, pcilib_dma_flags_t flags);

    int (*start_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
    int (*stop_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

    int (*push)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *written);
    int (*stream)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);

    double (*benchmark)(pcilib_dma_context_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);
};

int pcilib_set_dma_engine_description(pcilib_t *ctx, pcilib_dma_engine_t engine, pcilib_dma_engine_description_t *desc);

#endif /* _PCILIB_DMA_H */