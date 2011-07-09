#ifndef _PCILIB_DEFAULT_H
#define _PCILIB_DEFAULT_H

#include "pcilib.h"

int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);

#endif /* _PCILIB_DEFAULT_H */
