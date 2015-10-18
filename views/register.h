#ifndef _PCILIB_VIEW_REGISTER_H
#define _PCILIB_VIEW_REGISTER_H

#include <pcilib.h>
#include <pcilib/view.h>

typedef struct {
    pcilib_view_description_t base;
    const char *view;
    const char *reg;
    const char *bank;
} pcilib_register_view_description_t;


#ifndef _PCILIB_VIEW_REGISTER_C
extern const pcilib_view_api_description_t pcilib_register_view_api;
#endif /* _PCILIB_VIEW_REGISTER_C */

#endif /* _PCILIB_VIEW_REGISTER_H */
