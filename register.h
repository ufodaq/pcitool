#ifndef _PCILIB_REGISTER_H
#define _PCILIB_REGISTER_H

#include "pcilib.h"

struct pcilib_protocol_description_s {
    int (*read)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value);
    int (*write)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value);
};

    // we don't copy strings, they should be statically allocated
int pcilib_add_registers(pcilib_t *ctx, size_t n, pcilib_register_description_t *registers);

#endif /* _PCILIB_REGISTER_H */
