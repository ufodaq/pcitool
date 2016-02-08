#ifndef _PCITOOL_PCIPYWRAP_H
#define _PCITOOL_PCIPYWRAP_H
#include <Python.h>
#include "pcilib.h"

/*!
 * \brief Converts pcilib_value_t to PyObject.
 * \param ctx pointer to pcilib_t context
 * \param val pointer to pcilib_value_t to convert
 * \return PyObject, containing value. NULL with error message, sended to errstream.
 */
PyObject* pcilib_convert_val_to_pyobject(pcilib_t* ctx, pcilib_value_t *val, void (*errstream)(const char* msg, ...));


/*!
 * \brief Converts PyObject to pcilib_value_t.
 * \param ctx pcilib context
 * \param pyVal python object, containing value
 * \param val initialized polymorphic value
 * \return 0 on success or memory error
 */
int pcilib_convert_pyobject_to_val(pcilib_t* ctx, PyObject* pyVal, pcilib_value_t *val);


#endif /* _PCITOOL_PCIPYWRAP_H */
