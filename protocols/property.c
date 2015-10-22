#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>

#include "pci.h"
#include "tools.h"
#include "model.h"
#include "error.h"


int pcilib_property_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *regval) {
    int err;

    const pcilib_register_bank_description_t *b = bank_ctx->bank;
    int access = b->access / 8;

    pcilib_view_t view = addr / access;
    pcilib_value_t val = {0};

    if (addr % access) {
        pcilib_error("Can't perform unaligned access to property register (the address is 0x%lx and alligment requirement is %u)", addr, access);
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    if ((view == PCILIB_VIEW_INVALID)||(view >= ctx->num_views))
        return PCILIB_ERROR_INVALID_ARGUMENT;

    if ((ctx->views[view]->flags&PCILIB_VIEW_FLAG_REGISTER) == 0) {
        pcilib_error("Accessing invalid register %u (associated view %u does not provide register functionality)", addr, view);
        return PCILIB_ERROR_INVALID_REQUEST;
    }

    if ((ctx->views[view]->mode&PCILIB_ACCESS_R) == 0) {
        pcilib_error("Read access is not allowed to register %u (view %u)", addr, view);
        return PCILIB_ERROR_NOTPERMITED;
    }

    err = pcilib_get_property(ctx, ctx->views[view]->name, &val);
    if (err) return err;

    *regval = pcilib_get_value_as_register_value(ctx, &val, &err);

    return err;
}


int pcilib_property_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t regval) {
    int err;

    const pcilib_register_bank_description_t *b = bank_ctx->bank;
    int access = b->access / 8;

    pcilib_view_t view = addr / access;
    pcilib_value_t val = {0};

    if (addr % access) {
        pcilib_error("Can't perform unaligned access to property register (the address is 0x%lx and alligment requirement is %u)", addr, access);
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    if ((view == PCILIB_VIEW_INVALID)||(view >= ctx->num_views))
        return PCILIB_ERROR_INVALID_ARGUMENT;


    if ((ctx->views[view]->flags&PCILIB_VIEW_FLAG_REGISTER) == 0) {
        pcilib_error("Accessing invalid register %u (associated view %u does not provide register functionality)", addr, view);
        return PCILIB_ERROR_INVALID_REQUEST;
    }

    if ((ctx->views[view]->mode&PCILIB_ACCESS_W) == 0) {
        pcilib_error("Write access is not allowed to register %u (view %u)", addr, view);
        return PCILIB_ERROR_NOTPERMITED;
    }


    err = pcilib_set_value_from_register_value(ctx, &val, regval);
    if (err) return err;

    return pcilib_set_property(ctx, ctx->views[view]->name, &val);
}
