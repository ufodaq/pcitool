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

#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_PY_H */
