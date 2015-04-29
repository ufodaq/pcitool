#define _PCILIB_EXPORT_C

#include <stdio.h>

#include "export.h"

#include "protocols/default.h"


const pcilib_register_protocol_description_t pcilib_protocols[] = {
    { PCILIB_REGISTER_PROTOCOL_DEFAULT, &pcilib_default_protocol_api, NULL, NULL, "default", "" },
    { 0 }
};

#include "dma/nwl.h"
#include "dma/ipe.h"


const pcilib_dma_description_t pcilib_ipedma = 
    { &ipe_dma_api, ipe_dma_banks, ipe_dma_registers, ipe_dma_engines, NULL, NULL, "ipedma", "DMA engine developed by M. Caselle" };

const pcilib_dma_description_t pcilib_nwldma =
    { &nwl_dma_api, nwl_dma_banks, nwl_dma_registers, NULL, NULL, NULL, "nwldma", "North West Logic DMA Engine" };

const pcilib_dma_description_t pcilib_dma[] = { 
    { &ipe_dma_api, ipe_dma_banks, ipe_dma_registers, ipe_dma_engines, NULL, NULL, "ipedma", "DMA engine developed by M. Caselle" },
    { &nwl_dma_api, nwl_dma_banks, nwl_dma_registers, NULL, NULL, NULL, "nwldma", "North West Logic DMA Engine" },
    { &nwl_dma_api, nwl_dma_banks, nwl_dma_registers, NULL, "ipecamera", NULL, "nwldma-ipe", "North West Logic DMA Engine" },
    { 0 }
};


