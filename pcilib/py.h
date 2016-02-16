#ifndef _PCILIB_PY_H
#define _PCILIB_PY_H

#include "pcilib.h"

typedef struct pcilib_py_s pcilib_py_t;
typedef void pcilib_py_object;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_init_py(pcilib_t *ctx);
int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value);
void pcilib_free_py(pcilib_t *ctx);


int pcilib_py_init_script(pcilib_t *ctx, char* module_name, pcilib_access_mode_t *mode);
int pcilib_py_free_script(pcilib_t *ctx,char* module_name);
int pcilib_script_run_func(pcilib_t *ctx, char* module_name,
                           const char* func_name,  pcilib_value_t *val);

/*!
 * \brief Converts pcilib_value_t to PyObject.
 * \param ctx pointer to pcilib_t context
 * \param val pointer to pcilib_value_t to convert
 * \return PyObject, containing value. NULL with error message, sended to errstream.
 */
pcilib_py_object* pcilib_get_value_as_pyobject(pcilib_t* ctx, pcilib_value_t *val, int *err);


/*!
 * \brief Converts PyObject to pcilib_value_t.
 * \param ctx pcilib context
 * \param pyVal python object, containing value
 * \param val initialized polymorphic value
 * \return 0 on success or memory error
 */
int pcilib_set_value_from_pyobject(pcilib_t* ctx, pcilib_value_t *val, pcilib_py_object* pyObjVal);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_PY_H */
