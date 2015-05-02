#ifndef _PCILIB_DMA_H
#define _PCILIB_DMA_H

#include <pcilib.h>
#include <pcilib/bank.h>
#include <pcilib/register.h>

#define PCILIB_DMA_BUFFER_INVALID ((size_t)-1)

typedef struct {
    int used;					/**< Indicates if buffer has unread data or empty and ready for DMA */
    int error;					/**< Indicates if data is complete and correctly transfered or some error occured during the DMA transfer */
    int first;					/**< Indicates the first buffer of the packet */
    int last;					/**< Indicates the last buffer of the packet */
    size_t size;				/**< Indicates number of bytes actually written to the buffer */
} pcilib_dma_buffer_status_t;

typedef struct {
    int started;				/**< Informs if the engine is currently started or not */
    size_t ring_size, buffer_size;		/**< The number of allocated DMA buffers and size of each buffer in bytes */
    size_t ring_head, ring_tail;		/**< The first and the last buffer containing the data */
} pcilib_dma_engine_status_t;

typedef enum {
    PCILIB_DMA_TYPE_BLOCK,			/**< Simple DMA engine */
    PCILIB_DMA_TYPE_PACKET,			/**< Streaming (scatter-gather) DMA engine */
    PCILIB_DMA_TYPE_UNKNOWN
} pcilib_dma_engine_type_t;

typedef struct {
    pcilib_dma_engine_addr_t addr;		/**< Address of DMA engine (from 0) */
    pcilib_dma_engine_type_t type;		/**< Type of DMA engine */
    pcilib_dma_direction_t direction;		/**< Defines which kind of transfer does engine support: C2S, S2C, or both */
    size_t addr_bits;				/**< Number of addressable bits in the system memory (we currently work with 32-bits only) */

    const char *name;				/**< Defines a short name of engine (its main use, i.e. main, auxilliary, etc.) */
    const char *description;			/**< Defines a longer description of the engine */
} pcilib_dma_engine_description_t;

/*
typedef struct {
    int ignore_eop;
} pcilib_dma_parameters_t;
*/

typedef struct {
//    pcilib_dma_parameters_t params;
    pcilib_t *pcilib;				/**< Reference to the pcilib context */
} pcilib_dma_context_t;

typedef struct {
    pcilib_version_t version;

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
    const pcilib_dma_api_description_t *api;				/**< Defines all API functions for DMA operation */
    const pcilib_register_bank_description_t *banks;			/**< Pre-defined register banks exposed by DMA interface, additional banks can be defined during DMA initialization */
    const pcilib_register_description_t *registers;			/**< Pre-defined registers exposed by DMA interface, additional registers can be defined during DMA initialization */
    const pcilib_dma_engine_description_t *engines;			/**< List of DMA engines exposed by DMA interface, alternatively engines can be added during DMA initialization */
    const char *model;							/**< If NULL, the actually used event model is used instead */
    const void *args;							/**< Custom DMA-specific arguments. The actual structure may depend on the specified model */
    const char *name;							/**< Short name of DMA interface */
    const char *description;						/**< A bit longer description of DMA interface */
} pcilib_dma_description_t;


const pcilib_dma_description_t *pcilib_get_dma_description(pcilib_t *ctx);
pcilib_dma_engine_t pcilib_add_dma_engine(pcilib_t *ctx, pcilib_dma_engine_description_t *desc);
int pcilib_get_dma_status(pcilib_t *ctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers);
int pcilib_init_dma(pcilib_t *ctx);


#endif /* _PCILIB_DMA_H */
