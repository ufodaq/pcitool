#ifndef _PCILIB_DEFAULT_H
#define _PCILIB_DEFAULT_H

#include "pcilib.h"
#include "model.h"

int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#ifdef _PCILIB_EXPORT_C
const pcilib_register_protocol_api_description_t pcilib_default_protocol_api =
    { NULL, NULL, pcilib_default_read, pcilib_default_write };
#endif /* _PCILIB_EXPORT_C */

#endif /* _PCILIB_DEFAULT_H */
