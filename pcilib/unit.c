#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "pcilib.h"
#include "unit.h"
#include "error.h"


pcilib_unit_t pcilib_find_unit_by_name(pcilib_t *ctx, const char *unit) {
    pcilib_unit_t i;

    for(i = 0; ctx->units[i].name; i++) {
        if (!strcasecmp(ctx->units[i].name, unit))
	    return i;
    }
    return PCILIB_UNIT_INVALID;
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
