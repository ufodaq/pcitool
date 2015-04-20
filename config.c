#define _PCILIB_CONFIG_C

#include <stdio.h>

#include "model.h"
#include "error.h"
#include "config.h"

#include "protocols/default.h"


void (*pcilib_error)(const char *msg, ...) = pcilib_print_error;
void (*pcilib_warning)(const char *msg, ...) = pcilib_print_error;


const pcilib_register_protocol_description_t pcilib_protocols[] = {
    { PCILIB_REGISTER_PROTOCOL_DEFAULT, &pcilib_default_protocol_api, NULL, NULL, "default", "" },
    { 0 }
};

//static const pcilib_register_protocol_description_t *pcilib_protocol_default = NULL;//{0};//&pcilib_protocols[0];

#include "dma/nwl.h"
#include "dma/ipe.h"


/*
pcilib_register_protocol_alias_t pcilib_protocols[] = {
    { "default", 	{ &pcilib_default_protocol_api, PCILIB_REGISTER_PROTOCOL_MODIFICATION_DEFAULT, NULL } },
    { NULL, 		{0} }
};
*/


const pcilib_dma_description_t pcilib_dma[] = {
    { &ipe_dma_api, ipe_dma_banks, ipe_dma_registers, ipe_dma_engines, NULL, NULL, "ipedma", "DMA engine developed by M. Caselle" },
    { &nwl_dma_api, nwl_dma_banks, nwl_dma_registers, NULL, NULL, NULL, "nwldma", "North West Logic DMA Engine" },
    { &nwl_dma_api, nwl_dma_banks, nwl_dma_registers, NULL, "ipecamera", NULL, "nwldma-ipe", "North West Logic DMA Engine" },
    { 0 }
};
