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


static void pcilib_enum_view_free(pcilib_t *ctx, pcilib_view_description_t *view) {
    pcilib_enum_view_description_t *v = (pcilib_enum_view_description_t*)view;
    if (v->names) 
        free(v->names);
    free(v);
}

static int pcilib_enum_view_read(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t regval, pcilib_value_t *val) {
    int i;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_enum_view_description_t *v = (pcilib_enum_view_description_t*)(model_info->views[view_ctx->view]);

    for (i = 0; v->names[i].name; i++) {
        if ((regval >= v->names[i].min)&&(regval <= v->names[i].max))
            return pcilib_set_value_from_static_string(ctx, val, v->names[i].name);
    }

    return pcilib_set_value_from_int(ctx, val, regval);
}

static int pcilib_enum_view_write(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t *regval, const pcilib_value_t *val) {
    int i;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_enum_view_description_t *v = (pcilib_enum_view_description_t*)(model_info->views[view_ctx->view]);

    if (val->type != PCILIB_TYPE_STRING) {
        pcilib_warning("Value of unsupported type (%s) is passed to enum_view", pcilib_get_type_name(val->type));
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    for (i = 0; v->names[i].name; i++) {
        if (!strcasecmp(v->names[i].name, val->sval)) {
            *regval = v->names[i].value;
            return 0;
        }
    }

    pcilib_warning("Error setting register value, the value corresponding to name (%s) is not defined", val->sval);
    return PCILIB_ERROR_NOTFOUND;
}

const pcilib_view_api_description_t pcilib_enum_view_static_api =
  { PCILIB_VERSION, sizeof(pcilib_enum_view_description_t), NULL, NULL, NULL, pcilib_enum_view_read,  pcilib_enum_view_write };
const pcilib_view_api_description_t pcilib_enum_view_xml_api =
  { PCILIB_VERSION, sizeof(pcilib_enum_view_description_t), NULL, NULL, pcilib_enum_view_free,  pcilib_enum_view_read,  pcilib_enum_view_write };
