#ifndef _PCILIB_PY_H
#define _PCILIB_PY_H

typedef struct pcilib_py_s pcilib_py_t;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_init_py(pcilib_t *ctx);
int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value);
void pcilib_free_py(pcilib_t *ctx);

#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_PY_H */
