#ifndef _PCILIB_DMA_IPE_PRIVATE_H
#define _PCILIB_DMA_IPE_PRIVATE_H

#include "dma.h"

//#define IPEDMA_ENFORCE_64BIT_MODE	1		/**< enforce 64-bit mode addressing (otherwise it is used only if register 0x18 specifies PCIe gen3 as required by DMA engine) */
#define IPEDMA_CORES			1
#define IPEDMA_MAX_TLP_SIZE		256		/**< Defines maximum TLP in bytes supported by device */
//#define IPEDMA_TLP_SIZE			128		/**< If set, enforces the specified TLP size */

#define IPEDMA_STREAMING_MODE				/**< Enables streaming DMA operation mode instead of ring-buffer, the page is written once and forgotten and need to be pushed in queue again */
//#define IPEDMA_STREAMING_CHECKS				/**< Enables status checks in streaming mode (it will cause _significant_ performance penalty, max ~ 2 GB/s) */
#define IPEDMA_DMA_PROGRESS_THRESHOLD	1		/**< how many pages the DMA engine should fill before reporting progress */
#define IPEDMA_DESCRIPTOR_SIZE		128
#define IPEDMA_DESCRIPTOR_ALIGNMENT	64


//#define IPEDMA_BUG_LAST_READ				/**< We should forbid writting the second last available DMA buffer (the last is forbidden by design) */
//#define IPEDMA_DETECT_PACKETS				/**< Using empty_deceted flag */
#define  IPEDMA_SUPPORT_EMPTY_DETECTED			/**< Avoid waiting for data when empty_detected flag is set in hardware */

#define IPEDMA_REG_ADDR_MASK 0xFFF
#define IPEDMA_REG_BANK_MASK 0xF000
#define IPEDMA_REG_BANK_SHIFT 12

#define REG2VIRT(reg) (ctx->base_addr[(reg&IPEDMA_REG_BANK_MASK)>>IPEDMA_REG_BANK_SHIFT] + (reg&IPEDMA_REG_ADDR_MASK))
#define REG(bank, addr) ((bank<<IPEDMA_REG_BANK_SHIFT)|addr)
#define CONFREG(addr) REG(2, addr)

#define IPEDMA_REG_VERSION		REG(1, 0x20)

#define IPEDMA_REG_RESET		0x00
#define IPEDMA_REG_CONTROL		0x04
#define IPEDMA_REG_TLP_SIZE		0x0C
#define IPEDMA_REG_TLP_COUNT		0x10
#define IPEDMA_REG_PCIE_GEN		0x18
#define IPEDMA_REG_UPDATE_THRESHOLD	0x60
#define IPEDMA_REG_STREAMING_STATUS	0x68

    // PCIe gen2
#define IPEDMA_REG2_PAGE_ADDR		0x50
#define IPEDMA_REG2_UPDATE_ADDR		0x54
#define IPEDMA_REG2_LAST_READ		0x58		/**< In streaming mode, we can use it freely to track current status */
#define IPEDMA_REG2_PAGE_COUNT		0x5C

    // PCIe gen3
#define IPEDMA_REG3_PAGE_COUNT		0x40		/**< Read-only now */
#define IPEDMA_REG3_PAGE_ADDR		0x50
#define IPEDMA_REG3_UPDATE_ADDR		0x58
#define IPEDMA_REG3_LAST_READ		CONFREG(0x80)


#define IPEDMA_FLAG_NOSYNC		0x01		/**< Do not call kernel space for page synchronization */
#define IPEDMA_FLAG_NOSLEEP		0x02		/**< Do not sleep in the loop while waiting for the data */

#define IPEDMA_MASK_PCIE_GEN		0xF
#define IPEDMA_MASK_STREAMING_MODE	0x10

#define IPEDMA_RESET_DELAY		10000		/**< Sleep between accessing DMA control and reset registers */
#define IPEDMA_ADD_PAGE_DELAY		1000		/**< Delay between submitting successive DMA pages into IPEDMA_REG_PAGE_ADDR register */
#define IPEDMA_NODATA_SLEEP		100		/**< To keep CPU free, in nanoseconds */


#define WR(addr, value) { *(uint32_t*)(REG2VIRT(addr)) = value; }
#define RD(addr, value) { value = *(uint32_t*)(REG2VIRT(addr)); }

#define WR64(addr, value) { *(uint64_t*)(REG2VIRT(addr)) = value; }
#define RD64(addr, value) { value = *(uint64_t*)(REG2VIRT(addr)); }

#define DEREF(ptr) ((ctx->version<3)?(*(uint32_t*)ptr):(*(uint64_t*)ptr))

typedef uint32_t reg_t;
typedef struct ipe_dma_s ipe_dma_t;

struct ipe_dma_s {
    pcilib_dma_context_t dmactx;
    //pcilib_dma_engine_description_t engine[2];

    const pcilib_register_bank_description_t *dma_bank;
    char *base_addr[3];

    pcilib_irq_type_t irq_enabled;	/**< indicates that IRQs are enabled */
    pcilib_irq_type_t irq_preserve;	/**< indicates that IRQs should not be disabled during clean-up */
    int irq_started;			/**< indicates that IRQ subsystem is initialized (detecting which types should be preserverd) */    

    uint32_t version;			/**< hardware version */
    int started;			/**< indicates that DMA buffers are initialized and reading is allowed */
    int writting;			/**< indicates that we are in middle of writting packet */
    int reused;				/**< indicates that DMA was found intialized, buffers were reused, and no additional initialization is needed */
    int preserve;			/**< indicates that DMA should not be stopped during clean-up */
    int mode64;				/**< indicates 64-bit operation mode */
    int streaming;			/**< indicates if DMA is operating in streaming or ring-buffer mode */

    uint32_t dma_flags;			/**< Various operation flags, see IPEDMA_FLAG_* */
    size_t dma_timeout;			/**< DMA timeout,IPEDMA_DMA_TIMEOUT is used by default */
    size_t dma_pages;			/**< Number of DMA pages in ring buffer to allocate */
    size_t dma_page_size;		/**< Size of a single DMA page */

    pcilib_kmem_handle_t *desc;		/**< in-memory status descriptor written by DMA engine upon operation progess */
    pcilib_kmem_handle_t *pages;	/**< collection of memory-locked pages for DMA operation */

    size_t ring_size, page_size;
    size_t last_read, last_written;
    uintptr_t last_read_addr;

    reg_t reg_last_read;		/**< actual location of last_read register (removed from hardware for version 3) */
};

#endif /* _PCILIB_DMA_IPE_PRIVATE_H */
