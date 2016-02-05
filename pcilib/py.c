#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "debug.h"
#include "pcilib.h"
#include "py.h"
#include "error.h"

struct pcilib_py_s {
    PyObject *main_module;
    PyObject *global_dict;
};

int pcilib_init_py(pcilib_t *ctx) {
    ctx->py = (pcilib_py_t*)malloc(sizeof(pcilib_py_t));
    if (!ctx->py) return PCILIB_ERROR_MEMORY;

    Py_Initialize();

    ctx->py->main_module = PyImport_AddModule("__parser__");
    if (!ctx->py->main_module)
        return PCILIB_ERROR_FAILED;

    ctx->py->global_dict = PyModule_GetDict(ctx->py->main_module);
    if (!ctx->py->global_dict)
        return PCILIB_ERROR_FAILED;

    return 0;
}

void pcilib_free_py(pcilib_t *ctx) {
    if (ctx->py) {
	    // Dict and module references are borrowed
        free(ctx->py);
        ctx->py = NULL;
    }

    Py_Finalize();
}

/*
static int pcilib_py_realloc_string(pcilib_t *ctx, size_t required, size_t *size, char **str) {
    char *ptr;
    size_t cur = *size;
    
    if ((required + 1) > cur) {
        while (cur < required) cur *= 2;
        ptr = (char*)realloc(*str, cur);
        if (!ptr) return PCILIB_ERROR_MEMORY;
        *size = cur;
        *str = ptr;
    }
    ]
    return 0;
}
*/

static char *pcilib_py_parse_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value) {
    int i;
    int err = 0;
    pcilib_value_t val = {0};
    pcilib_register_value_t regval;

    char save;
    char *reg, *cur;
    size_t offset = 0;

    size_t size;
    char *src;
    char *dst;

    size_t max_repl = 2 + 2 * sizeof(pcilib_value_t);                               // the text representation of largest integer

    if (value) {
        err = pcilib_copy_value(ctx, &val, value);
        if (err) return NULL;

        err = pcilib_convert_value_type(ctx, &val, PCILIB_TYPE_STRING);
        if (err) return NULL;

        if (strlen(val.sval) > max_repl)
            max_repl = strlen(val.sval);
    }

    size = ((max_repl + 1) / 3 + 1) * strlen(codestr);                          // minimum register length is 3 symbols ($a + delimiter space), it is replaces with (max_repl+1) symbols

    src = strdup(codestr);
    dst = (char*)malloc(size);                                                  // allocating maximum required space

    if ((!src)||(!dst)) {
        if (src) free(src);
        if (dst) free(dst);
        pcilib_error("Failed to allocate memory for string formulas");
        return NULL;
    }

    cur = src;
    reg = strchr(src, '$');
    while (reg) {
        strcpy(dst + offset, cur);
        offset += reg - cur;

            // find the end of the register name
        reg++;
        if (*reg == '{') {
            reg++;
            for (i = 0; (reg[i])&&(reg[i] != '}'); i++);
            if (!reg[i]) {
                pcilib_error("Python formula (%s) contains unterminated variable reference", codestr);
                err = PCILIB_ERROR_INVALID_DATA;
                break;
            }
        } else {
            for (i = 0; isalnum(reg[i])||(reg[i] == '_'); i++);
        }
        save = reg[i];
        reg[i] = 0;

            // determine replacement value
        if (!strcasecmp(reg, "value")) {
            if (!value) {
                pcilib_error("Python formula (%s) relies on the value of register, but it is not provided", codestr);
                err = PCILIB_ERROR_INVALID_REQUEST;
                break;
            }

            strcpy(dst + offset, val.sval);
        } else {
            if (*reg == '/') {
                pcilib_value_t val = {0};
                err = pcilib_get_property(ctx, reg, &val);
                if (err) break;
                err = pcilib_convert_value_type(ctx, &val, PCILIB_TYPE_STRING);
                if (err) break;
                sprintf(dst + offset, "%s", val.sval);
            } else {
                err = pcilib_read_register(ctx, NULL, reg, &regval);
                if (err) break;
                sprintf(dst + offset, "0x%lx", regval);
            }
        }

        offset += strlen(dst + offset);
        if (save == '}') i++;
        else reg[i] = save;

            // Advance to the next register if any
        cur = reg + i;
        reg = strchr(cur, '$');
    }

    strcpy(dst + offset, cur);

    free(src);

    if (err) {
        free(dst);
        return NULL;
    }

    return dst;
}

int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value) {
    PyGILState_STATE gstate;
    char *code;
    PyObject* obj;

    code = pcilib_py_parse_string(ctx, codestr, value);
    if (!code) {
        pcilib_error("Failed to parse registers in the code: %s", codestr);
        return PCILIB_ERROR_FAILED;
    }

    gstate = PyGILState_Ensure();
    obj = PyRun_String(code, Py_eval_input, ctx->py->global_dict, ctx->py->global_dict);
    PyGILState_Release(gstate);

    if (!obj) {
        pcilib_error("Failed to run the Python code: %s", code);
        return PCILIB_ERROR_FAILED;
    }

    pcilib_debug(VIEWS, "Evaluating a Python string \'%s\' to %lf=\'%s\'", codestr, PyFloat_AsDouble(obj), code);
    return pcilib_set_value_from_float(ctx, value, PyFloat_AsDouble(obj));
}
