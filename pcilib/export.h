#ifndef _PCILIB_EXPORT_H
#define _PCILIB_EXPORT_H


#include <pcilib/register.h>
#include <pcilib/bank.h>
#include <pcilib/dma.h>

extern const pcilib_register_protocol_description_t pcilib_protocols[];
extern const pcilib_dma_description_t pcilib_dma[];

extern const pcilib_register_protocol_api_description_t pcilib_default_protocol_api;

extern const pcilib_dma_description_t pcilib_ipedma;
extern const pcilib_dma_description_t pcilib_nwldma;

#endif /* _PCILIB_EXPORT_H */
