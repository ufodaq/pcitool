#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>

#include "tools.h"
#include "model.h"
#include "error.h"
#include "bar.h"
#include "datacpy.h"
#include "pci.h"

#define default_datacpy(dst, src, access, bank)   pcilib_datacpy(dst, src, access, 1, bank->raw_endianess)

uintptr_t pcilib_default_resolve(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_address_resolution_flags_t flags, pcilib_register_addr_t reg_addr) {
    uintptr_t addr;
    const pcilib_register_bank_description_t *b = bank_ctx->bank;

    if (reg_addr == PCILIB_REGISTER_ADDRESS_INVALID) reg_addr = 0;

    switch (flags&PCILIB_ADDRESS_RESOLUTION_MASK_ACCESS_MODE) {
     case 0:
	if (b->read_addr != b->write_addr)
	    return PCILIB_ADDRESS_INVALID;
     case PCILIB_ADDRESS_RESOLUTION_FLAG_READ_ONLY:
	addr = b->read_addr + reg_addr;
	break;
     case PCILIB_ADDRESS_RESOLUTION_FLAG_WRITE_ONLY:
        addr = b->write_addr + reg_addr;
     default:
        return PCILIB_ADDRESS_INVALID;
    }

    switch (flags&PCILIB_ADDRESS_RESOLUTION_MASK_ADDRESS_TYPE) {
     case 0:
        return (uintptr_t)pcilib_resolve_bar_address(ctx, b->bar, addr);
     case PCILIB_ADDRESS_RESOLUTION_FLAG_BUS_ADDRESS:
     case PCILIB_ADDRESS_RESOLUTION_FLAG_PHYS_ADDRESS:
        return ctx->board_info.bar_start[b->bar] + addr;
    }

    return PCILIB_ADDRESS_INVALID;
}

int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value) {
    char *ptr;
    pcilib_register_value_t val = 0;
    
    const pcilib_register_bank_description_t *b = bank_ctx->bank;

    int access = b->access / 8;

    ptr =  pcilib_resolve_bar_address(ctx, b->bar, b->read_addr + addr);
    default_datacpy(&val, ptr, access, b);

//    *value = val&BIT_MASK(bits);
    *value = val;

    return 0;
}


int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value) {
    char *ptr;

    const pcilib_register_bank_description_t *b = bank_ctx->bank;

    int access = b->access / 8;

    ptr =  pcilib_resolve_bar_address(ctx, b->bar, b->write_addr + addr);
    default_datacpy(ptr, &value, access, b);

    return 0;
}
