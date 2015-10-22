#define _PCILIB_EXPORT_C

#include <stdio.h>

#include "export.h"


const char *pcilib_data_types[] = { "default", "string", "double", "long" };


#include "protocols/default.h"
#include "protocols/software.h"
#include "protocols/property.h"

const pcilib_register_protocol_description_t pcilib_standard_register_protocols[] = {
    { PCILIB_REGISTER_PROTOCOL_DEFAULT, &pcilib_default_protocol_api, NULL, NULL, "default", "" },
    { PCILIB_REGISTER_PROTOCOL_SOFTWARE, &pcilib_software_protocol_api, NULL, NULL, "software_registers", "" },
    { PCILIB_REGISTER_PROTOCOL_PROPERTY, &pcilib_property_protocol_api, NULL, NULL, "property_registers", "" },
    { 0 }
};

const pcilib_register_bank_description_t pcilib_standard_register_banks[] = {
    { PCILIB_REGISTER_BANK_CONF, PCILIB_REGISTER_PROTOCOL_SOFTWARE, PCILIB_BAR_NOBAR, 0, 0, 32, 0, PCILIB_HOST_ENDIAN, PCILIB_HOST_ENDIAN, "%lu", "conf", "pcilib configuration"},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL }
};

const pcilib_register_description_t pcilib_standard_registers[] = {
    {0x0000, 	0, 	32, 	PCILIB_VERSION, 	0x00000000,	PCILIB_REGISTER_R   , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_CONF, "version",  		"Version"},
    {0x0004, 	0, 	32, 	0, 			0x00000000,	PCILIB_REGISTER_RW  , PCILIB_REGISTER_STANDARD, PCILIB_REGISTER_BANK_CONF, "max_threads",	"Limits number of threads used by event engines (0 - unlimited)"},
    {0,		0,	0,	0,			0x00000000,	0,                                           0,                        	0, NULL, 		NULL}
};


const pcilib_register_bank_description_t pcilib_property_register_bank = 
    { PCILIB_REGISTER_BANK_PROPERTY, PCILIB_REGISTER_PROTOCOL_PROPERTY, PCILIB_BAR_NOBAR, 0, 0, 8 * sizeof(pcilib_register_value_t), 0, PCILIB_HOST_ENDIAN, PCILIB_HOST_ENDIAN, "%lu", "property", "Computed registers interfacing properties"};


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


