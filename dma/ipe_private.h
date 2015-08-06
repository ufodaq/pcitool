#ifndef _PCILIB_DMA_IPE_PRIVATE_H
#define _PCILIB_DMA_IPE_PRIVATE_H

#include "dma.h"

//#define IPEDMA_ENFORCE_64BIT_MODE	1		/**< enforce 64-bit mode addressing (otherwise it is used only if register 0x18 specifies PCIe gen3 as required by DMA engine) */
#define IPEDMA_CORES			1
#define IPEDMA_MAX_TLP_SIZE		256		/**< Defines maximum TLP in bytes supported by device */
//#define IPEDMA_TLP_SIZE			128		/**< If set, enforces the specified TLP size */

#define IPEDMA_STREAMING_MODE				/**< Enables streaming DMA operation mode instead of ring-buffer, the page is written once and forgotten and need to be pushed in queue again */
#define IPEDMA_STREAMING_CHECKS				/**< Enables status checks in streaming mode (it will cause performance penalty) */
#define IPEDMA_PAGE_SIZE		4096
#define IPEDMA_DMA_PAGES		1024		/**< number of DMA pages in the ring buffer to allocate */
#define IPEDMA_DMA_PROGRESS_THRESHOLD	1		/**< how many pages the DMA engine should fill before reporting progress */
#define IPEDMA_DESCRIPTOR_SIZE		128
#define IPEDMA_DESCRIPTOR_ALIGNMENT	64


//#define IPEDMA_BUG_LAST_READ				/**< We should forbid writting the second last available DMA buffer (the last is forbidden by design) */
//#define IPEDMA_DETECT_PACKETS				/**< Using empty_deceted flag */
#define  IPEDMA_SUPPORT_EMPTY_DETECTED			/**< Avoid waiting for data when empty_detected flag is set in hardware */

#define IPEDMA_DMA_TIMEOUT 		100000		/**< us, overrides PCILIB_DMA_TIMEOUT (actual hardware timeout is 50ms according to Lorenzo) */
#define IPEDMA_RESET_DELAY		100000		/**< Sleep between accessing DMA control and reset registers */
#define IPEDMA_ADD_PAGE_DELAY		1000		/**< Delay between submitting successive DMA pages into IPEDMA_REG_PAGE_ADDR register */
#define IPEDMA_NODATA_SLEEP		10		/**< To keep CPU free */

#define IPEDMA_REG_RESET		0x00
#define IPEDMA_REG_CONTROL		0x04
#define IPEDMA_REG_TLP_SIZE		0x0C
#define IPEDMA_REG_TLP_COUNT		0x10
#define IPEDMA_REG_PCIE_GEN		0x18
#define IPEDMA_REG_PAGE_ADDR		0x50
#define IPEDMA_REG_UPDATE_ADDR		0x54
#define IPEDMA_REG_LAST_READ		0x58		/**< In streaming mode, we can use it freely to track current status */
#define IPEDMA_REG_PAGE_COUNT		0x5C
#define IPEDMA_REG_UPDATE_THRESHOLD	0x60
#define IPEDMA_REG_STREAMING_STATUS	0x68

#define IPEDMA_MASK_PCIE_GEN		0xF
#define IPEDMA_MASK_STREAMING_MODE	0x10


#define WR(addr, value) { *(uint32_t*)(ctx->base_addr + addr) = value; }
#define RD(addr, value) { value = *(uint32_t*)(ctx->base_addr + addr); }


typedef struct ipe_dma_s ipe_dma_t;

struct ipe_dma_s {
    pcilib_dma_context_t dmactx;
    //pcilib_dma_engine_description_t engine[2];

    const pcilib_register_bank_description_t *dma_bank;
    char *base_addr;

    pcilib_irq_type_t irq_enabled;	/**< indicates that IRQs are enabled */
    pcilib_irq_type_t irq_preserve;	/**< indicates that IRQs should not be disabled during clean-up */
    int irq_started;			/**< indicates that IRQ subsystem is initialized (detecting which types should be preserverd) */    

    int started;			/**< indicates that DMA buffers are initialized and reading is allowed */
    int writting;			/**< indicates that we are in middle of writting packet */
    int reused;				/**< indicates that DMA was found intialized, buffers were reused, and no additional initialization is needed */
    int preserve;			/**< indicates that DMA should not be stopped during clean-up */
    int mode64;				/**< indicates 64-bit operation mode */
    int streaming;			/**< indicates if DMA is operating in streaming or ring-buffer mode */

    pcilib_kmem_handle_t *desc;		/**< in-memory status descriptor written by DMA engine upon operation progess */
    pcilib_kmem_handle_t *pages;	/**< collection of memory-locked pages for DMA operation */

    size_t ring_size, page_size;
    size_t last_read, last_read_addr, last_written;

};

#endif /* _PCILIB_DMA_IPE_PRIVATE_H */
