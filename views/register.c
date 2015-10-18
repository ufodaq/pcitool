#define _PCILIB_VIEW_ENUM_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <uthash.h>

#include "error.h"
#include "version.h"
#include "model.h"
#include "enum.h"
#include "view.h"
#include "value.h"
#include "register.h"


static void pcilib_register_view_free(pcilib_t *ctx, pcilib_view_description_t *view) {
    if (view->name)
        free((void*)view->name);
    free(view);
}

static int pcilib_register_view_read(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t dummy, pcilib_value_t *val) {
    int err;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_register_view_description_t *v = (pcilib_register_view_description_t*)(model_info->views[view_ctx->view]);

    if (v->view) {
        return pcilib_read_register_view(ctx, v->bank, v->reg, v->view, val);
    } else {
        pcilib_register_value_t regval;

        err = pcilib_read_register(ctx, v->bank, v->reg, &regval);
        if (err) return err;

        return pcilib_set_value_from_register_value(ctx, val, regval);
    }
}

static int pcilib_register_view_write(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t *dummy, const pcilib_value_t *val) {
    int err;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_register_view_description_t *v = (pcilib_register_view_description_t*)(model_info->views[view_ctx->view]);

    if (v->view) {
        return pcilib_write_register_view(ctx, v->bank, v->reg, v->view, val);
    } else {
        pcilib_register_value_t regval;

        regval = pcilib_get_value_as_register_value(ctx, val, &err);
        if (err) return err;

        return pcilib_write_register(ctx, v->bank, v->reg, regval);
    }

    return err;
}

const pcilib_view_api_description_t pcilib_register_view_api =
  { PCILIB_VERSION, sizeof(pcilib_register_view_description_t), NULL, NULL, pcilib_register_view_free,  pcilib_register_view_read,  pcilib_register_view_write };
