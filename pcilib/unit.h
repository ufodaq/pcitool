#ifndef _PCILIB_UNIT_H
#define _PCILIB_UNIT_H

#include <pcilib.h>

#define PCILIB_UNIT_INVALID ((pcilib_unit_t)-1)
#define PCILIB_UNIT_TRANSFORM_INVALID ((pcilib_unit_transform_t)-1)

#define PCILIB_MAX_TRANSFORMS_PER_UNIT 16			/**< Limits number of supported transforms per unit */

typedef struct pcilib_unit_context_s pcilib_unit_context_t;

/**
 * unit transformation routines
 */
typedef struct {
    char *unit;								                /**< Name of the resulting unit */
    char *transform;							                /**< String, similar to view formula, explaining transform to this unit */
} pcilib_unit_transform_t;

typedef struct {
    char *name;										/**< Unit name */
    pcilib_unit_transform_t transforms[PCILIB_MAX_TRANSFORMS_PER_UNIT + 1];		/**< Transforms to other units */
} pcilib_unit_description_t;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_description_t *desc);
void pcilib_clean_units(pcilib_t *ctx);

pcilib_unit_t pcilib_find_unit_by_name(pcilib_t *ctx, const char *unit);
pcilib_unit_transform_t *pcilib_find_transform_by_unit_names(pcilib_t *ctx, const char *from, const char *to);

    // value is modified
int pcilib_transform_unit(pcilib_t *ctx, pcilib_unit_transform_t *trans, pcilib_value_t *value);
int pcilib_transform_unit_by_name(pcilib_t *ctx, const char *to, pcilib_value_t *value);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_UNIT_H */
