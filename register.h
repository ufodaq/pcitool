#ifndef _PCILIB_REGISTER_H
#define _PCILIB_REGISTER_H

#include "pcilib.h"

struct pcilib_protocol_description_s {
    int (*read)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value);
    int (*write)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value);
};

#endif /* _PCILIB_REGISTER_H */
