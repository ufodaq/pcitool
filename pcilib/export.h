#ifndef _PCILIB_EXPORT_H
#define _PCILIB_EXPORT_H


#include <pcilib/register.h>
#include <pcilib/bank.h>
#include <pcilib/dma.h>

extern const char *pcilib_data_types[];

extern const pcilib_register_protocol_description_t pcilib_standard_register_protocols[];
extern const pcilib_register_bank_description_t pcilib_standard_register_banks[];
extern const pcilib_register_description_t pcilib_standard_registers[];

extern const pcilib_dma_description_t pcilib_dma[];

extern const pcilib_register_protocol_api_description_t pcilib_default_protocol_api;

extern const pcilib_register_bank_description_t pcilib_property_register_bank;

extern const pcilib_dma_description_t pcilib_ipedma;
extern const pcilib_dma_description_t pcilib_nwldma;

#endif /* _PCILIB_EXPORT_H */
