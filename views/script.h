#ifndef _PCILIB_VIEW_SCRIPT_H
#define _PCILIB_VIEW_SCRIPT_H

#include <pcilib.h>
#include <pcilib/view.h>
#include <Python.h>

typedef struct {
    pcilib_view_description_t base;
    PyObject *py_script_module;	/**< PyModule object, contains script enviroment */
	char* script_name;
} pcilib_script_view_description_t;

#ifndef _PCILIB_VIEW_SCRIPT_C
const pcilib_view_api_description_t pcilib_script_view_api;
#endif /* _PCILIB_VIEW_SCRIPT_C */

#endif /* _PCILIB_VIEW_SCRIPT_H */
