#ifndef _PCILIB_NWL_H
#define _PCILIB_NWL_H

typedef struct nwl_dma_s nwl_dma_t;
typedef struct pcilib_nwl_engine_description_s pcilib_nwl_engine_description_t;

#define NWL_DMA_IRQ_SOURCE 0

#define NWL_XAUI_ENGINE 0
#define NWL_XRAWDATA_ENGINE 1
#define NWL_FIX_EOP_FOR_BIG_PACKETS		// requires precise sizes in read requests
//#define NWL_GENERATE_DMA_IRQ

#define PCILIB_NWL_ALIGNMENT 			64  // in bytes
#define PCILIB_NWL_DMA_DESCRIPTOR_SIZE		64  // in bytes
#define PCILIB_NWL_DMA_PAGES			512 // 1024

#include "nwl_dma.h"
#include "nwl_irq.h"
#include "nwl_register.h"
#include "nwl_engine.h"
#include "nwl_loopback.h"

#define nwl_read_register(var, ctx, base, reg) pcilib_datacpy(&var, base + reg, 4, 1, ctx->dma_bank->raw_endianess)
#define nwl_write_register(var, ctx, base, reg) pcilib_datacpy(base + reg, &var, 4, 1, ctx->dma_bank->raw_endianess)

struct pcilib_nwl_engine_description_s {
    pcilib_dma_engine_description_t desc;
    char *base_addr;
    
    size_t ring_size, page_size;
    size_t head, tail;
    pcilib_kmem_handle_t *ring;
    pcilib_kmem_handle_t *pages;
    
    int started;			/**< indicates that DMA buffers are initialized and reading is allowed */
    int writting;			/**< indicates that we are in middle of writting packet */
    int preserve;			/**< indicates that DMA should not be stopped during clean-up */
};


struct nwl_dma_s {
    pcilib_t *pcilib;
    
    pcilib_register_bank_description_t *dma_bank;
    char *base_addr;

    pcilib_irq_type_t irq_enabled;	/**< indicates that IRQs are enabled */
    pcilib_irq_type_t irq_preserve;	/**< indicates that IRQs should not be disabled during clean-up */
    int started;			/**< indicates that DMA subsystem is initialized and DMA engine can start */
    int irq_started;			/**< indicates that IRQ subsystem is initialized (detecting which types should be preserverd) */    
    int loopback_started;		/**< indicates that benchmarking subsystem is initialized */

    pcilib_dma_engine_t n_engines;
    pcilib_nwl_engine_description_t engines[PCILIB_MAX_DMA_ENGINES + 1];
};


#endif /* _PCILIB_NWL_H */
