#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>

#include "tools.h"
#include "model.h"
#include "error.h"

#define default_datacpy(dst, src, access, bank)   pcilib_datacpy(dst, src, access, 1, bank->raw_endianess)

int pcilib_default_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value) {
    char *ptr;
    pcilib_register_value_t val = 0;
    
    const pcilib_register_bank_description_t *b = bank_ctx->bank;
    printf("bank name %s\n",b->name);
    printf("bank bar %i\n",b->bar);
    printf("addr %i\n",addr);
    int access = b->access / 8;
    printf("access : %i\n",access);
    ptr =  pcilib_resolve_register_address(ctx, b->bar, b->read_addr + addr);
    // printf("ptr %s\n",ptr);
    default_datacpy(&val, ptr, access, b);
    printf("val : %i\n",val);
//    *value = val&BIT_MASK(bits);
    *value = val;
    printf("value : %i\n",*value);
    return 0;
}


int pcilib_default_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value) {
    char *ptr;

    const pcilib_register_bank_description_t *b = bank_ctx->bank;

    int access = b->access / 8;

    ptr =  pcilib_resolve_register_address(ctx, b->bar, b->write_addr + addr);
    default_datacpy(ptr, &value, access, b);

    return 0;
}
