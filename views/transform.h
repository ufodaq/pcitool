#ifndef _PCILIB_VIEW_TRANSFORM_H
#define _PCILIB_VIEW_TRANSFORM_H

#include <pcilib.h>
#include <pcilib/view.h>

typedef struct {
    pcilib_view_description_t base;
    const char *read_from_reg;			/**< Formula explaining how to convert the register value to the view value */
    const char *write_to_reg;			/**< Formula explaining how to convert from the view value to the register value */
} pcilib_transform_view_description_t;

#ifndef _PCILIB_VIEW_TRANSFORM_C
const pcilib_view_api_description_t pcilib_transform_view_api;
#endif /* _PCILIB_VIEW_TRANSFORM_C */

#endif /* _PCILIB_VIEW_TRANSFORM_H */
