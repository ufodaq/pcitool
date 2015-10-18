#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "pcilib.h"
#include "unit.h"
#include "error.h"

static pcilib_unit_transform_t pcilib_unit_transform_null = { NULL, NULL };

int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_description_t *desc) {
    size_t i;
    int err = 0;

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

        // ToDo: Check if exists...
    for (i = 0; i < n; i++) {
        pcilib_unit_t unit = pcilib_find_unit_by_name(ctx, desc[i].name);
        if (unit != PCILIB_UNIT_INVALID) {
            pcilib_clean_units(ctx, ctx->num_units);
            pcilib_error("Unit %s is already defined in the model", desc[i].name);
            return PCILIB_ERROR_EXIST;
        }

        pcilib_unit_context_t *unit_ctx = (pcilib_unit_context_t*)malloc(sizeof(pcilib_unit_context_t));
        if (!unit_ctx) {
            pcilib_clean_units(ctx, ctx->num_units);
            return PCILIB_ERROR_MEMORY;
        }

        memset(unit_ctx, 0, sizeof(pcilib_unit_context_t));
        unit_ctx->unit = ctx->num_units + i;
        unit_ctx->name = desc[i].name;

        HASH_ADD_KEYPTR(hh, ctx->unit_hash, unit_ctx->name, strlen(unit_ctx->name), unit_ctx);
        memcpy(ctx->units + ctx->num_units + i, &desc[i], sizeof(pcilib_unit_description_t));
    }

    ctx->num_units += n;
    memset(ctx->units + ctx->num_units, 0, sizeof(pcilib_unit_description_t));

    return err;
}

void pcilib_clean_units(pcilib_t *ctx, pcilib_unit_t start) {
    pcilib_unit_context_t *s, *tmp;

    if (ctx->unit_hash) {
        HASH_ITER(hh, ctx->unit_hash, s, tmp) {
            if (s->unit >= start) {
                HASH_DEL(ctx->unit_hash, s);
                free(s);
            }
        }
    }

    memset(&ctx->units[start], 0, sizeof(pcilib_unit_description_t));
    ctx->num_units = start;
}

pcilib_unit_t pcilib_find_unit_by_name(pcilib_t *ctx, const char *name) {
    pcilib_unit_context_t *unit_ctx = NULL;

    HASH_FIND_STR(ctx->unit_hash, name, unit_ctx);
    if (unit_ctx) return unit_ctx->unit;

/*
    pcilib_unit_t i;
    for(i = 0; ctx->units[i].name; i++) {
        if (!strcasecmp(ctx->units[i].name, name))
	    return i;
    }
*/
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

int pcilib_transform_unit(pcilib_t *ctx, const pcilib_unit_transform_t *trans, pcilib_value_t *value) {
    int err;

    if (trans->transform) {
        err = pcilib_py_eval_string(ctx, trans->transform, value);
        if (err) return err;

        value->unit = trans->unit;
    } else if (trans->unit) {
        value->unit = trans->unit;
    } 

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
