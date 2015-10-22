#ifndef _PCILIB_UNIT_H
#define _PCILIB_UNIT_H

#include <uthash.h>

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

struct pcilib_unit_context_s {
    const char *name;
    pcilib_unit_t unit;
    UT_hash_handle hh;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Use this function to add new unit definitions into the model. It is error to re-register
 * already registered unit. The function will copy the context of unit description, but name, 
 * transform, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new units are initalized.
 * @param[in,out] ctx 	- pcilib context
 * @param[in] n 	- number of units to initialize. It is OK to pass 0 if protocols variable is NULL terminated (last member of protocols array have all members set to 0)
 * @param[in] desc 	- unit descriptions
 * @return 		- error or 0 on success
 */
int pcilib_add_units(pcilib_t *ctx, size_t n, const pcilib_unit_description_t *desc);

/**
 * Destroys data associated with units. This is an internal function and will
 * be called during clean-up.
 * @param[in,out] ctx 	- pcilib context
 * @param[in] start 	- specifies first unit to clean (used to clean only part of the units to keep the defined state if pcilib_add_units has failed)
 */
void pcilib_clean_units(pcilib_t *ctx, pcilib_unit_t start);

/**
 * Find unit id using supplied name.
 * @param[in] ctx 	- pcilib context
 * @param[in] unit	- the requested unit name
 * @return		- unit id or PCILIB_UNIT_INVALID if error not found or error has occured
 */
pcilib_unit_t pcilib_find_unit_by_name(pcilib_t *ctx, const char *unit);

/**
 * Find the required transform between two specified units.
 * @param[in] ctx 	- pcilib context
 * @param[in] from	- the source unit
 * @param[in] to	- destination unit
 * @return		- the pointer to unit transform or NULL in case of error. If no transform required (i.e. \a from = \a to), 
			the function will return #pcilib_unit_transform_t structure with \a transform field set to NULL.
 */
pcilib_unit_transform_t *pcilib_find_transform_by_unit_names(pcilib_t *ctx, const char *from, const char *to);

/**
 * Converts value to the requested units. It is error to convert values with unspecified units.
 * This is internal function, use pcilib_value_convert_value_unit instead.
 * @param[in,out] ctx 	- pcilib context
 * @param[in] trans 	- the requested unit transform 
 * @param[in,out] value - the value to be converted (changed on success)
 * @return 		- error or 0 on success
 */
int pcilib_transform_unit(pcilib_t *ctx, const pcilib_unit_transform_t *trans, pcilib_value_t *value);

/**
 * Converts value to the requested units. It is error to convert values with unspecified units.
 * This is internal function, use pcilib_value_convert_value_unit instead.
 * @param[in,out] ctx 	- pcilib context
 * @param[in] to 	- specifies the requested unit of the value
 * @param[in,out] value - the value to be converted (changed on success)
 * @return 		- error or 0 on success
 */
int pcilib_transform_unit_by_name(pcilib_t *ctx, const char *to, pcilib_value_t *value);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_UNIT_H */
