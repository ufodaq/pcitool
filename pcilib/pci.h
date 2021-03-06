#ifndef _PCITOOL_PCI_H
#define _PCITOOL_PCI_H

#define PCILIB_DEFAULT_CPU_COUNT 2
#define PCILIB_EVENT_TIMEOUT 1000000		/**< us */
#define PCILIB_TRIGGER_TIMEOUT 100000		/**< us */
#define PCILIB_DMA_TIMEOUT 10000		/**< us */
#define PCILIB_DMA_SKIP_TIMEOUT 1000000		/**< us */
#define PCILIB_MAX_BARS 6			/**< this is defined by PCI specification */
#define PCILIB_DEFAULT_REGISTER_SPACE 1024	/**< number of registers to allocate on init */
#define PCILIB_DEFAULT_VIEW_SPACE 128    	/**< number of views to allocate on init */
#define PCILIB_DEFAULT_UNIT_SPACE 128           /**< number of units to allocate on init */
#define PCILIB_MAX_REGISTER_BANKS 32		/**< maximum number of register banks to allocate space for */
#define PCILIB_MAX_REGISTER_RANGES 32		/**< maximum number of register ranges to allocate space for */
#define PCILIB_MAX_REGISTER_PROTOCOLS 32	/**< maximum number of register protocols to support */
#define PCILIB_MAX_DMA_ENGINES 32		/**< maximum number of supported DMA engines */

#include <uthash.h>

#include "linux-3.10.h"
#include "driver/ioctl.h"

#include "timing.h"
#include "cpu.h"
#include "bar.h"
#include "pcilib.h"
#include "register.h"
#include "kmem.h"
#include "irq.h"
#include "dma.h"
#include "event.h"
#include "model.h"
#include "export.h"
#include "locking.h"
#include "xml.h"
#include "py.h"
#include "view.h"
#include "memcpy.h"

typedef struct {
    uint8_t max_link_speed, link_speed;
    uint8_t max_link_width, link_width;
    uint8_t max_payload, payload;
} pcilib_pcie_link_info_t;


struct pcilib_s {
    int handle;										/**< file handle of device */

    pcilib_driver_version_t driver_version;						/**< Version reported by the driver */

    uintptr_t page_mask;								/**< Selects bits which define offset within the page */
    int board_info_ready;								/**< Flag indicating if board info is already requested and populated */
    pcilib_board_info_t board_info;							/**< The mandatory information about board as defined by PCI specification */
    pcilib_pcie_link_info_t link_info;							/**< Infomation about PCIe connection */
    char *bar_space[PCILIB_MAX_BARS];							/**< Pointers to the mapped BARs in virtual address space */
    pcilib_bar_info_t bar_info[PCILIB_MAX_BARS + 1];					/**< NULL terminated list of PCI bar descriptions */

    int pci_cfg_space_fd;								/**< File descriptor linking to PCI configuration space in sysfs */
    uint32_t pci_cfg_space_cache[64];							/**< Cached PCI configuration space */
    size_t pci_cfg_space_size;								/**< Size of the cached PCI configuration space, sometimes not fully is available for unpriveledged user */
    const uint32_t *pcie_capabilities;							/**< PCI Capbility structure (just a pointer at appropriate place in the pci_cfg_space) */

    int reg_bar_mapped;									/**< Indicates that all BARs used to access registers are mapped */
    pcilib_bar_t reg_bar;								/**< Default BAR to look for registers, other BARs will be looked as well */
    int data_bar_mapped;								/**< Indicates that a BAR for large PIO is mapped */
    pcilib_bar_t data_bar;								/**< BAR for large PIO operations */

    pcilib_kmem_list_t *kmem_list;							/**< List of currently allocated kernel memory */

    char *model;									/**< Requested model */
    void *event_plugin;									/**< Currently loaded event engine */
    pcilib_model_description_t model_info;						/**< Current model description combined from the information returned by the event plugin and all dynamic sources (XML, DMA registers, etc.). Contains only pointers, no deep copy of information returned by event plugin */

    size_t num_banks_init;								/**< Number of initialized banks */
    size_t num_reg, alloc_reg;								/**< Number of registered and allocated registers */
    size_t num_banks, num_protocols, num_ranges;					/**< Number of registered banks, protocols, and register ranges */
    size_t num_engines;									/**< Number of configured DMA engines */
    size_t dyn_banks;									/**< Number of configured dynamic banks */
    pcilib_register_description_t *registers;						/**< List of currently defined registers (from all sources) */
    pcilib_register_bank_description_t banks[PCILIB_MAX_REGISTER_BANKS + 1];		/**< List of currently defined register banks (from all sources) */
    pcilib_register_range_t ranges[PCILIB_MAX_REGISTER_RANGES + 1];			/**< List of currently defined register ranges (from all sources) */
    pcilib_register_protocol_description_t protocols[PCILIB_MAX_REGISTER_PROTOCOLS + 1];/**< List of currently defined register protocols (from all sources) */
    pcilib_dma_description_t dma;							/**< Configuration of used DMA implementation */
    pcilib_dma_engine_description_t engines[PCILIB_MAX_DMA_ENGINES +  1];		/**< List of engines defined by the DMA implementation */

    pcilib_register_context_t *register_ctx;						/**< Contexts for registers */
    pcilib_register_bank_context_t *bank_ctx[PCILIB_MAX_REGISTER_BANKS];		/**< Contexts for registers banks if required by register protocol */
    pcilib_dma_context_t *dma_ctx;							/**< DMA context */
    pcilib_context_t *event_ctx;							/**< Implmentation context */

    size_t num_views, alloc_views;							/**< Number of configured and allocated  views*/
    size_t num_units, alloc_units;							/**< Number of configured and allocated  units*/
    pcilib_view_description_t **views;							/**< list of currently defined views */
    pcilib_unit_description_t *units;							/**< list of currently defined units */

    pcilib_unit_context_t *unit_hash;                                                   /**< Hash of units */
    pcilib_view_context_t *view_hash;                                                   /**< Hash of views */
    pcilib_register_context_t *reg_hash;                                                /**< Hash of registers */

    pcilib_lock_t *dma_rlock[PCILIB_MAX_DMA_ENGINES];					/**< Per-engine locks to serialize streaming and read operations */
    pcilib_lock_t *dma_wlock[PCILIB_MAX_DMA_ENGINES];					/**< Per-engine locks to serialize write operations */

    struct pcilib_locking_s locks;							/**< Context of locking subsystem */
    struct pcilib_xml_s xml;                                                    	/**< XML context */
    struct pcilib_py_s *py;                                                              /**< Python execution context */

#ifdef PCILIB_FILE_IO
    int file_io_handle;
#endif /* PCILIB_FILE_IO */
};

#ifdef __cplusplus
extern "C" {
#endif

pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx);

const pcilib_driver_version_t *pcilib_get_driver_version(pcilib_t *ctx);
const pcilib_board_info_t *pcilib_get_board_info(pcilib_t *ctx);
const pcilib_pcie_link_info_t *pcilib_get_pcie_link_info(pcilib_t *ctx);
int pcilib_get_device_state(pcilib_t *ctx, pcilib_device_state_t *state);


int pcilib_map_register_space(pcilib_t *ctx);
int pcilib_map_data_space(pcilib_t *ctx, uintptr_t addr);

int pcilib_set_dma_mask(pcilib_t *ctx, int mask);
int pcilib_set_mps(pcilib_t *ctx, int mps);

#ifdef __cplusplus
}
#endif

#endif /* _PCITOOL_PCI_H */
