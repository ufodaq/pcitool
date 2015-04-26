#ifndef _PCILIB_DMA_NWL_PRIVATE_H
#define _PCILIB_DMA_NWL_PRIVATE_H

typedef struct nwl_dma_s nwl_dma_t;
typedef struct pcilib_nwl_engine_context_s pcilib_nwl_engine_context_t;

#define NWL_DMA_IRQ_SOURCE 0

#define NWL_XAUI_ENGINE 0
#define NWL_XRAWDATA_ENGINE 1
#define NWL_MAX_PACKET_SIZE 4096 //16384
//#define NWL_GENERATE_DMA_IRQ

#define PCILIB_NWL_ALIGNMENT 			64  // in bytes
#define PCILIB_NWL_DMA_DESCRIPTOR_SIZE		64  // in bytes
#define PCILIB_NWL_DMA_PAGES			256 // 1024

#define PCILIB_NWL_REGISTER_TIMEOUT 10000	/**< us */

//#define DEBUG_HARDWARE
//#define DEBUG_NWL

#include "nwl.h"
#include "nwl_irq.h"
#include "nwl_engine.h"
#include "nwl_loopback.h"

#define nwl_read_register(var, ctx, base, reg) pcilib_datacpy(&var, base + reg, 4, 1, ctx->dma_bank->raw_endianess)
#define nwl_write_register(var, ctx, base, reg) pcilib_datacpy(base + reg, &var, 4, 1, ctx->dma_bank->raw_endianess)


struct pcilib_nwl_engine_context_s {
    const pcilib_dma_engine_description_t *desc;
    char *base_addr;

    size_t ring_size, page_size;
    size_t head, tail;
    pcilib_kmem_handle_t *ring;
    pcilib_kmem_handle_t *pages;
    
    int started;			/**< indicates that DMA buffers are initialized and reading is allowed */
    int writting;			/**< indicates that we are in middle of writting packet */
    int reused;				/**< indicates that DMA was found intialized, buffers were reused, and no additional initialization is needed */
    int preserve;			/**< indicates that DMA should not be stopped during clean-up */
};

typedef enum {
    NWL_MODIFICATION_DEFAULT,
    NWL_MODIFICATION_IPECAMERA
} nwl_modification_t;

struct nwl_dma_s {
    pcilib_dma_context_t dmactx;

    nwl_modification_t type;
    int ignore_eop;			/**< always set end-of-packet */

    const pcilib_register_bank_description_t *dma_bank;
    char *base_addr;

    pcilib_irq_type_t irq_enabled;	/**< indicates that IRQs are enabled */
    pcilib_irq_type_t irq_preserve;	/**< indicates that IRQs should not be disabled during clean-up */
    int started;			/**< indicates that DMA subsystem is initialized and DMA engine can start */
    int irq_started;			/**< indicates that IRQ subsystem is initialized (detecting which types should be preserverd) */    
    int loopback_started;		/**< indicates that benchmarking subsystem is initialized */

//    pcilib_dma_engine_t n_engines;
    pcilib_nwl_engine_context_t engines[PCILIB_MAX_DMA_ENGINES + 1];
};

int nwl_add_registers(nwl_dma_t *ctx);

#endif /* _PCILIB_DMA_NWL_PRIVATE_H */
