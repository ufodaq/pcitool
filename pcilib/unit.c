#include "pcilib.h"
#include "pci.h"
#include "stdio.h"
#include <string.h>
#include "error.h"
#include "unit.h"

int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_t* units) {
	
    pcilib_unit_t *units2;
    size_t size;

    if (!n) {
	for (n = 0; units[n].name[0]; n++);
    }

    if ((ctx->num_units + n + 1) > ctx->alloc_units) {
      for (size = ctx->alloc_units; size < 2 * (n + ctx->num_units + 1); size<<=1);

	units2 = (pcilib_unit_t*)realloc(ctx->units, size * sizeof(pcilib_unit_t));
	if (!units2) return PCILIB_ERROR_MEMORY;

	ctx->units = units2;
	ctx->alloc_units = size;
    }

    memcpy(ctx->units + ctx->num_units, units, n * sizeof(pcilib_unit_t));
    memset(ctx->units + ctx->num_units + n, 0, sizeof(pcilib_unit_t));

    ctx->num_units += n;
    
    return 0;
}
    
