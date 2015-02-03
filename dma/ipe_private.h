#ifndef _PCILIB_DMA_IPE_PRIVATE_H
#define _PCILIB_DMA_IPE_PRIVATE_H

#define IPEDMA_CORES			1
#define IPEDMA_TLP_SIZE			32
#define IPEDMA_PAGE_SIZE		4096
#define IPEDMA_DMA_PAGES		16		/**< number of DMA pages in the ring buffer to allocate */
#define IPEDMA_DMA_PROGRESS_THRESHOLD	1		/**< how many pages the DMA engine should fill before reporting progress */
#define IPEDMA_DESCRIPTOR_SIZE		128
#define IPEDMA_DESCRIPTOR_ALIGNMENT	64

//#define IPEDMA_DEBUG
//#define IPEDMA_BUG_DMARD				/**< No register read during DMA transfer */
#define IPEDMA_DMA_TIMEOUT PCILIB_DMA_TIMEOUT		/**< us, overrides PCILIB_DMA_TIMEOUT */

#define IPEDMA_REG_RESET		0x00
#define IPEDMA_REG_CONTROL		0x04
#define IPEDMA_REG_TLP_SIZE		0x0C
#define IPEDMA_REG_TLP_COUNT		0x10
#define IPEDMA_REG_PAGE_ADDR		0x50
#define IPEDMA_REG_UPDATE_ADDR		0x54
#define IPEDMA_REG_LAST_READ		0x58
#define IPEDMA_REG_PAGE_COUNT		0x5C
#define IPEDMA_REG_UPDATE_THRESHOLD	0x60



typedef struct ipe_dma_s ipe_dma_t;

struct ipe_dma_s {
    struct pcilib_dma_context_s dmactx;
    pcilib_dma_engine_description_t engine[2];

    pcilib_t *pcilib;
    
    pcilib_register_bank_description_t *dma_bank;
    char *base_addr;

    pcilib_irq_type_t irq_enabled;	/**< indicates that IRQs are enabled */
    pcilib_irq_type_t irq_preserve;	/**< indicates that IRQs should not be disabled during clean-up */
    int irq_started;			/**< indicates that IRQ subsystem is initialized (detecting which types should be preserverd) */    

    int started;			/**< indicates that DMA buffers are initialized and reading is allowed */
    int writting;			/**< indicates that we are in middle of writting packet */
    int reused;				/**< indicates that DMA was found intialized, buffers were reused, and no additional initialization is needed */
    int preserve;			/**< indicates that DMA should not be stopped during clean-up */
    int mode64;				/**< indicates 64-bit operation mode */

    pcilib_kmem_handle_t *desc;		/**< in-memory status descriptor written by DMA engine upon operation progess */
    pcilib_kmem_handle_t *pages;	/**< collection of memory-locked pages for DMA operation */

    size_t ring_size, page_size;
    size_t last_read, last_read_addr, last_written;

};

#endif /* _PCILIB_DMA_IPE_PRIVATE_H */
