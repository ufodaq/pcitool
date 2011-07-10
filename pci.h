#ifndef _PCITOOL_PCI_H
#define _PCITOOL_PCI_H

#define PCILIB_DMA_TIMEOUT 10000		/**< us */
#define PCILIB_REGISTER_TIMEOUT 10000		/**< us */

#include "driver/pciDriver.h"
#include "pcilib_types.h"

#include "pcilib.h"
#include "register.h"
#include "kmem.h"
#include "irq.h"
#include "dma.h"
#include "event.h"

struct pcilib_s {
    int handle;
    
    uintptr_t page_mask;
    pcilib_board_info_t board_info;
    pcilib_dma_info_t dma_info;
    pcilib_model_t model;
    
    char *bar_space[PCILIB_MAX_BANKS];

    int reg_bar_mapped;
    pcilib_bar_t reg_bar;
//    char *reg_space;

    int data_bar_mapped;
    pcilib_bar_t data_bar;
//    char *data_space;
//    size_t data_size;
    
    pcilib_kmem_list_t *kmem_list;

    size_t num_reg, alloc_reg;
    pcilib_model_description_t model_info;
    
    pcilib_dma_context_t *dma_ctx;
    pcilib_context_t *event_ctx;
    
#ifdef PCILIB_FILE_IO
    int file_io_handle;
#endif /* PCILIB_FILE_IO */
};

#ifdef _PCILIB_PCI_C
# include "ipecamera/model.h"
# include "dma/nwl.h"
# include "default.h"

pcilib_model_description_t pcilib_model[3] = {
    { 4, PCILIB_HOST_ENDIAN, 	NULL, NULL, NULL, NULL, NULL },
    { 4, PCILIB_HOST_ENDIAN, 	NULL, NULL, NULL, NULL, NULL },
    { 4, PCILIB_LITTLE_ENDIAN,	ipecamera_registers, ipecamera_register_banks, ipecamera_register_ranges, ipecamera_events, &nwl_dma_api, &ipecamera_image_api }
};

pcilib_protocol_description_t pcilib_protocol[3] = {
    { pcilib_default_read, pcilib_default_write },
    { ipecamera_read, ipecamera_write },
    { NULL, NULL }
};
#else
extern pcilib_model_description_t pcilib_model[];

extern void (*pcilib_error)(const char *msg, ...);
extern void (*pcilib_warning)(const char *msg, ...);

extern pcilib_protocol_description_t pcilib_protocol[];
#endif /* _PCILIB_PCI_C */

const pcilib_board_info_t *pcilib_get_board_info(pcilib_t *ctx);
const pcilib_dma_info_t *pcilib_get_dma_info(pcilib_t *ctx);

int pcilib_map_register_space(pcilib_t *ctx);
int pcilib_map_data_space(pcilib_t *ctx, uintptr_t addr);


#endif /* _PCITOOL_PCI_H */
