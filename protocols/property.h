#ifndef _PCILIB_PROTOCOL_PROPERTY_H
#define _PCILIB_PROTOCOL_PROPERTY_H

#include "pcilib.h"
#include "version.h"
#include "model.h"

int pcilib_property_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int pcilib_property_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#ifdef _PCILIB_EXPORT_C
const pcilib_register_protocol_api_description_t pcilib_property_protocol_api =
    { PCILIB_VERSION, NULL, NULL, NULL, pcilib_property_registers_read, pcilib_property_registers_write };
#endif /* _PCILIB_EXPORT_C */

#endif /* _PCILIB_PROTOCOL_PROPERTY_H */
