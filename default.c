#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>

#include "tools.h"
#include "default.h"
#include "error.h"

#define default_datacpy(dst, src, access, bank)   pcilib_datacpy(dst, src, access, 1, bank->raw_endianess)

int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *value) {
    int err;
    
    char *ptr;
    pcilib_register_value_t val = 0;
    int access = bank->access / 8;

    ptr =  pcilib_resolve_register_address(ctx, bank->bar, bank->read_addr + addr * access);
    default_datacpy(&val, ptr, access, bank);
    
//    *value = val&BIT_MASK(bits);
    *value = val;

    return 0;
}


int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t value) {
    int err;
    
    char *ptr;
    int access = bank->access / 8;

    ptr =  pcilib_resolve_register_address(ctx, bank->bar, bank->write_addr + addr * access);
    default_datacpy(ptr, &value, access, bank);

    return 0;
}
