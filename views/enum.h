#ifndef _PCILIB_VIEW_ENUM_H
#define _PCILIB_VIEW_ENUM_H

#include <pcilib.h>
#include <pcilib/view.h>


typedef struct {
    pcilib_view_description_t base;
    pcilib_register_value_name_t *names;
} pcilib_enum_view_description_t;


#ifndef _PCILIB_VIEW_ENUM_C
extern const pcilib_view_api_description_t pcilib_enum_view_static_api;
extern const pcilib_view_api_description_t pcilib_enum_view_xml_api;
#endif /* _PCILIB_VIEW_ENUM_C */

#endif /* _PCILIB_VIEW_ENUM_H */
