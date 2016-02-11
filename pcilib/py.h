#ifndef _PCILIB_PY_H
#define _PCILIB_PY_H

#include "pcilib.h"

typedef struct pcilib_py_s pcilib_py_t;
typedef struct pcilib_script_s pcilib_script_t;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_init_py(pcilib_t *ctx);
int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value);
void pcilib_free_py(pcilib_t *ctx);


int pcilib_init_py_script(pcilib_t *ctx, char* module_name, pcilib_script_t **module, pcilib_access_mode_t *mode);
int pcilib_free_py_script(pcilib_script_t *module);
int pcilib_script_read(pcilib_t *ctx, pcilib_script_t *module, pcilib_value_t *val);
int pcilib_script_write(pcilib_t *ctx, pcilib_script_t *module, pcilib_value_t *val);


/*!
 * \brief Converts pcilib_value_t to PyObject.
 * \param ctx pointer to pcilib_t context
 * \param val pointer to pcilib_value_t to convert
 * \return PyObject, containing value. NULL with error message, sended to errstream.
 */
void* pcilib_convert_val_to_pyobject(pcilib_t* ctx, pcilib_value_t *val);


/*!
 * \brief Converts PyObject to pcilib_value_t.
 * \param ctx pcilib context
 * \param pyVal python object, containing value
 * \param val initialized polymorphic value
 * \return 0 on success or memory error
 */
int pcilib_convert_pyobject_to_val(pcilib_t* ctx, void* pyVal, pcilib_value_t *val);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_PY_H */
