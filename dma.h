#ifndef _PCILIB_DMA_H
#define _PCILIB_DMA_H

#define PCILIB_DMA_BUFFER_INVALID ((size_t)-1)


typedef struct {
    int started;
    size_t ring_size, buffer_size;
    size_t ring_head, ring_tail;
} pcilib_dma_engine_status_t;

typedef enum {
    PCILIB_DMA_TYPE_BLOCK,
    PCILIB_DMA_TYPE_PACKET,
    PCILIB_DMA_TYPE_UNKNOWN
} pcilib_dma_engine_type_t;

typedef struct {
    pcilib_dma_engine_addr_t addr;
    pcilib_dma_engine_type_t type;
    pcilib_dma_direction_t direction;
    size_t addr_bits;

    const char *name;
    const char *description;
} pcilib_dma_engine_description_t;

typedef struct {
    int used;
    int error;
    int first;
    int last;
    size_t size;
} pcilib_dma_buffer_status_t;

/*
typedef struct {
    int ignore_eop;
} pcilib_dma_parameters_t;
*/

typedef struct {
//    pcilib_dma_parameters_t params;
    pcilib_t *pcilib;
} pcilib_dma_context_t;

typedef struct {
    pcilib_dma_context_t *(*init)(pcilib_t *ctx, const char *model, const void *arg);
    void (*free)(pcilib_dma_context_t *ctx);
    
    int (*status)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);

    int (*enable_irq)(pcilib_dma_context_t *ctx, pcilib_irq_type_t irq_type, pcilib_dma_flags_t flags);
    int (*disable_irq)(pcilib_dma_context_t *ctx, pcilib_dma_flags_t flags);
    int (*acknowledge_irq)(pcilib_dma_context_t *ctx, pcilib_irq_type_t irq_type, pcilib_irq_source_t irq_source);

    int (*start_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);
    int (*stop_dma)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags);

    int (*push)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, void *buf, size_t *written);
    int (*stream)(pcilib_dma_context_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr);

    double (*benchmark)(pcilib_dma_context_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);
} pcilib_dma_api_description_t;


typedef struct {
    const pcilib_dma_api_description_t *api;
    const pcilib_register_bank_description_t *banks;
    const pcilib_register_description_t *registers;
    const pcilib_dma_engine_description_t *engines;
    const char *model;
    const void *args;
    const char *name;
    const char *description;
} pcilib_dma_description_t;


const pcilib_dma_description_t *pcilib_get_dma_info(pcilib_t *ctx);
pcilib_dma_engine_t pcilib_add_dma_engine(pcilib_t *ctx, pcilib_dma_engine_description_t *desc);
int pcilib_get_dma_status(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);
int pcilib_init_dma(pcilib_t *ctx);


#endif /* _PCILIB_DMA_H */
