#ifndef _PCILIB_VIEW_ENUM_H
#define _PCILIB_VIEW_ENUM_H

#include <pcilib.h>
#include <pcilib/view.h>

typedef struct {
    pcilib_register_value_t value, min, max;	/**< the value or value-range for which the name is defined, while wrtiting the name the value will be used even if range is specified */
    const char *name; 				/**< corresponding string to value */
} pcilib_value_name_t;

typedef struct {
    pcilib_view_description_t base;
    pcilib_value_name_t *names;
} pcilib_enum_view_description_t;


#ifndef _PCILIB_VIEW_ENUM_C
extern const pcilib_view_api_description_t pcilib_enum_view_static_api;
extern const pcilib_view_api_description_t pcilib_enum_view_xml_api;
#endif /* _PCILIB_VIEW_ENUM_C */

#endif /* _PCILIB_VIEW_ENUM_H */
