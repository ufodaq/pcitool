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

struct pcilib_script_s {
    PyObject *py_script_module;	/**< PyModule object, contains script enviroment */
	PyObject *dict;
	char* script_name;
};

int pcilib_init_py(pcilib_t *ctx) {
    ctx->py = (pcilib_py_t*)malloc(sizeof(pcilib_py_t));
    if (!ctx->py) return PCILIB_ERROR_MEMORY;

    if(!Py_IsInitialized())
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
    //Py_Finalize();
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

void* pcilib_convert_val_to_pyobject(pcilib_t* ctx, pcilib_value_t *val)
{
	int err;
	
	switch(val->type)
	{
		case PCILIB_TYPE_INVALID:
                pcilib_error("Invalid register output type (PCILIB_TYPE_INVALID)");
			return NULL;
			
		case PCILIB_TYPE_STRING:
                pcilib_error("Invalid register output type (PCILIB_TYPE_STRING)");
			return NULL;
		
		case PCILIB_TYPE_LONG:
		{
			long ret;
			ret = pcilib_get_value_as_int(ctx, val, &err);
			
			if(err)
			{
                    pcilib_error("Failed: pcilib_get_value_as_int (%i)", err);
				return NULL;
			}
			return (PyObject*)PyInt_FromLong((long) ret);
		}
		
		case PCILIB_TYPE_DOUBLE:
		{
			double ret;
			ret = pcilib_get_value_as_float(ctx, val, &err);
			
			if(err)
			{
                    pcilib_error("Failed: pcilib_get_value_as_int (%i)", err);
				return NULL;
			}
			return (PyObject*)PyFloat_FromDouble((double) ret);
		}
		
		default:
                pcilib_error("Invalid register output type (unknown)");
			return NULL;
	}
}

int pcilib_convert_pyobject_to_val(pcilib_t* ctx, void* pyObjVal, pcilib_value_t *val)
{
	PyObject* pyVal = pyObjVal;
	int err;
	
    if(PyInt_Check(pyVal))
    {
        err = pcilib_set_value_from_int(ctx, val, PyInt_AsLong(pyVal));
    }
    else
        if(PyFloat_Check(pyVal))
            err = pcilib_set_value_from_float(ctx, val, PyFloat_AsDouble(pyVal));
        else
            if(PyString_Check(pyVal))
                err = pcilib_set_value_from_static_string(ctx, val, PyString_AsString(pyVal));
                else
                {
                    pcilib_error("Invalid input. Input type should be int, float or string.");
                    return PCILIB_ERROR_NOTSUPPORTED;
                }
    if(err)
        return err;
        
    return 0;
}

int pcilib_init_py_script(pcilib_t *ctx, char* module_name, pcilib_script_t **module, pcilib_access_mode_t *mode)
{	
	//Initialize python script, if it has not initialized already.
	if(!module_name)
	{
		pcilib_error("Invalid script name specified in XML property (NULL)");
		return PCILIB_ERROR_INVALID_DATA;
	}
	
	//create path string to scripts
	char* model_dir = getenv("PCILIB_MODEL_DIR");
	char* model_path = malloc(strlen(model_dir) + strlen(ctx->model) + 2);
	if (!model_path) return PCILIB_ERROR_MEMORY;
	sprintf(model_path, "%s/%s", model_dir, ctx->model);
	
	//set model path to python
	PySys_SetPath(model_path);
	free(model_path);
	model_path = NULL;
	
	//create path string to pcipywrap library
	char* app_dir = getenv("APP_PATH");
	char* pcipywrap_path;
	if(app_dir)
	{
		pcipywrap_path = malloc(strlen(app_dir) + strlen("/pywrap"));
		if (!pcipywrap_path) return PCILIB_ERROR_MEMORY;
		sprintf(pcipywrap_path, "%s/%s", "/pywrap", ctx->model);
	}
	else
	{
		pcipywrap_path = malloc(strlen("./pywrap"));
		if (!pcipywrap_path) return PCILIB_ERROR_MEMORY;
		sprintf(pcipywrap_path, "%s", "./pywrap");

	}
	
	//set pcipywrap library path to python
	PyObject* path = PySys_GetObject("path");
	if(PyList_Append(path, PyString_FromString(pcipywrap_path)) == -1)
	{
		pcilib_error("Cant set pcipywrap library path to python.");
		return PCILIB_ERROR_FAILED;
	}
	free(pcipywrap_path);
	pcipywrap_path = NULL;
	
	
	//extract module name from script name
	char* py_module_name = strtok(module_name, ".");
	
	if(!py_module_name)
	{
		pcilib_error("Invalid script name specified in XML property (%s)."
					 " Seems like name doesnt contains extension", module_name);
		return PCILIB_ERROR_INVALID_DATA;
	}
	
	//import python script
	PyObject* py_script_module = PyImport_ImportModule(py_module_name);
	
	if(!py_script_module)
	{
		printf("Error in import python module: ");
		PyErr_Print();
		return PCILIB_ERROR_INVALID_DATA;
	}


	//Initializing pcipywrap module if script use it
	PyObject* dict = PyModule_GetDict(py_script_module);
	if(PyDict_Contains(dict, PyString_FromString("pcipywrap")))
	{
		PyObject* pcipywrap_module = PyDict_GetItemString(dict, "pcipywrap");
		if(!pcipywrap_module)
		{
			pcilib_error("Cant extract pcipywrap module from script dictionary");
			return PCILIB_ERROR_FAILED;
		}
	   
	   //setting pcilib_t instance                 
	   PyObject_CallMethodObjArgs(pcipywrap_module,
							   PyUnicode_FromString("setPcilib"),
							   PyByteArray_FromStringAndSize((const char*)&ctx, sizeof(pcilib_t*)),
							   NULL);
	}
	
	//Success. Create struct and initialize values
	module[0] = (pcilib_script_t*)malloc(sizeof(pcilib_script_t));
	module[0]->py_script_module = py_script_module;
	module[0]->script_name = module_name;
	module[0]->dict = dict;
	
	//Setting correct mode
	mode[0] = 0;
	if(PyDict_Contains(dict, PyString_FromString("read_from_register")))
		mode[0] |= PCILIB_ACCESS_R;	
	if(PyDict_Contains(dict, PyString_FromString("write_to_register")))
		mode[0] |= PCILIB_ACCESS_W;
	
	return 0;
}

int pcilib_free_py_script(pcilib_script_t *module)
{
	if(module)
	{
		if(module->script_name)
		{
			free(module->script_name);
			module->script_name = NULL;
		}
		
		if(module->py_script_module)
		{
			//PyObject_Free(module->py_script_module);
			module->py_script_module = NULL;
		}
	}
	
	return 0;
}

int pcilib_script_read(pcilib_t *ctx, pcilib_script_t *module, pcilib_value_t *val)
{
	
	int err;
	
	PyObject *ret = PyObject_CallMethod(module->py_script_module, "read_from_register", "()");
	if (!ret) 
	{
	   printf("Python script error: ");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}
	
	err = pcilib_convert_pyobject_to_val(ctx, ret, val);
	
	if(err)
	{
		pcilib_error("Failed to convert python script return value to internal type: %i", err);
		return err;
	}
    return 0;
}

int pcilib_script_write(pcilib_t *ctx, pcilib_script_t *module, pcilib_value_t *val)
{	
    PyObject *input = pcilib_convert_val_to_pyobject(ctx, val);
	if(!input)
	{
	   printf("Failed to convert input value to Python object");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}
	
	PyObject *ret = PyObject_CallMethodObjArgs(module->py_script_module,
											   PyUnicode_FromString("write_to_register"),
											   input,
											   NULL);
	if (!ret) 
	{
	   printf("Python script error: ");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}

    return 0;
}
