#ifndef _PCILIB_PROTOCOL_DEFAULT_H
#define _PCILIB_PROTOCOL_DEFAULT_H

#include "pcilib.h"
#include "version.h"
#include "model.h"

uintptr_t pcilib_default_resolve(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_address_resolution_flags_t flags, pcilib_register_addr_t addr);
int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#ifdef _PCILIB_EXPORT_C
const pcilib_register_protocol_api_description_t pcilib_default_protocol_api =
    { PCILIB_VERSION, NULL, NULL, pcilib_default_resolve, pcilib_default_read, pcilib_default_write };
#endif /* _PCILIB_EXPORT_C */

#endif /* _PCILIB_PROTOCOL_DEFAULT_H */
