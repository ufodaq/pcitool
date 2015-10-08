#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "pcilib.h"
#include "unit.h"
#include "error.h"


static pcilib_unit_transform_t pcilib_unit_transform_null = { NULL, NULL };

pcilib_unit_t pcilib_find_unit_by_name(pcilib_t *ctx, const char *name) {
    pcilib_unit_t i;

    for(i = 0; ctx->units[i].name; i++) {
        if (!strcasecmp(ctx->units[i].name, name))
	    return i;
    }
    return PCILIB_UNIT_INVALID;
}

pcilib_unit_transform_t *pcilib_find_transform_by_unit_names(pcilib_t *ctx, const char *from, const char *to) {
    int i;
    pcilib_unit_t unit;

    if ((!from)||(!to))
	return NULL;

    if (!strcasecmp(from, to)) 
	return &pcilib_unit_transform_null;

    unit = pcilib_find_unit_by_name(ctx, from);
    if (unit == PCILIB_UNIT_INVALID) return NULL;

    for (i = 0; ctx->units[unit].transforms[i].unit; i++) {
        if (!strcasecmp(ctx->units[unit].transforms[i].unit, to))
            return &ctx->units[unit].transforms[i];
    }

    return NULL;
}

int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_description_t *desc) {
    if (!n) {
        for (n = 0; desc[n].name; n++);
    }

    if ((ctx->num_units + n + 1) > ctx->alloc_units) {
	size_t size;
	pcilib_unit_description_t *units;

        for (size = ctx->alloc_units; size < 2 * (n + ctx->num_units + 1); size <<= 1);

        units = (pcilib_unit_description_t*)realloc(ctx->units, size * sizeof(pcilib_unit_description_t));
        if (!units) return PCILIB_ERROR_MEMORY;

        ctx->units = units;
        ctx->alloc_units = size;

        ctx->model_info.units = units;
    }

    memcpy(ctx->units + ctx->num_units, desc, n * sizeof(pcilib_unit_description_t));
    memset(ctx->units + ctx->num_units + n, 0, sizeof(pcilib_unit_description_t));

    ctx->num_units += n;

    return 0;
}


int pcilib_transform_unit(pcilib_t *ctx, pcilib_unit_transform_t *trans, pcilib_value_t *value) {
    int err;

    err = pcilib_py_eval_string(ctx, trans->transform, value);
    if (err) return err;

    value->unit = trans->unit;
    return 0;
}


int pcilib_transform_unit_by_name(pcilib_t *ctx, const char *to, pcilib_value_t *value) {
    pcilib_unit_transform_t *trans;

    if (!value->unit) {
        pcilib_warning("Can't transform unit of the value with unspecified unit");
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    trans = pcilib_find_transform_by_unit_names(ctx, value->unit, to);
    if (!trans) {
        pcilib_warning("Can't transform unit (%s) to (%s)", value->unit, to);
        return PCILIB_ERROR_NOTSUPPORTED;
    }

    return pcilib_transform_unit(ctx, trans, value);
}
