#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>

#include "pci.h"
#include "tools.h"
#include "model.h"
#include "error.h"


int pcilib_property_registers_read(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t *regval) {
    int err;

    pcilib_view_t view = addr;
    pcilib_value_t val = {0};

    if ((view == PCILIB_VIEW_INVALID)||(view >= ctx->num_views))
        return PCILIB_ERROR_INVALID_ARGUMENT;

    err = pcilib_get_property(ctx, ctx->views[view]->name, &val);
    if (err) return err;

    *regval = pcilib_get_value_as_register_value(ctx, &val, &err);

    return err;
}


int pcilib_property_registers_write(pcilib_t *ctx, pcilib_register_bank_context_t *bank, pcilib_register_addr_t addr, pcilib_register_value_t regval) {
    int err;

    pcilib_view_t view = addr;
    pcilib_value_t val = {0};

    if ((view == PCILIB_VIEW_INVALID)||(view >= ctx->num_views))
        return PCILIB_ERROR_INVALID_ARGUMENT;

    err = pcilib_set_value_from_register_value(ctx, &val, regval);
    if (err) return err;

    return pcilib_set_property(ctx, ctx->views[view]->name, &val);
}
