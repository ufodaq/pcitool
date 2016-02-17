#ifdef BUILD_PYTHON_MODULES
#include <Python.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "debug.h"
#include "pcilib.h"
#include "py.h"
#include "error.h"

#ifdef BUILD_PYTHON_MODULES
typedef struct pcilib_script_s {
	const char* name;
    PyObject *module;	/**< PyModule object, contains script enviroment */	
	UT_hash_handle hh;
} pcilib_script_s;

struct pcilib_py_s {
    PyObject *main_module;
    PyObject *global_dict;
    int py_initialized_inside; ///< Flag, shows that Py_Initialize has been called inside class
    struct pcilib_script_s *scripts;
};
#endif

int pcilib_init_py(pcilib_t *ctx) {
#ifdef BUILD_PYTHON_MODULES
    ctx->py = (pcilib_py_t*)malloc(sizeof(pcilib_py_t));
    if (!ctx->py) return PCILIB_ERROR_MEMORY;

    if(!Py_IsInitialized())
    {
        Py_Initialize();
        
        //Since python is being initializing from c programm, it needs
        //to initialize threads to works properly with c threads
        PyEval_InitThreads();
        PyEval_ReleaseLock();
        
        ctx->py->py_initialized_inside = 1;
	}
	else
		ctx->py->py_initialized_inside = 0;
		
    ctx->py->main_module = PyImport_AddModule("__parser__");
    if (!ctx->py->main_module)
        return PCILIB_ERROR_FAILED;

    ctx->py->global_dict = PyModule_GetDict(ctx->py->main_module);
    if (!ctx->py->global_dict)
        return PCILIB_ERROR_FAILED;
	
	
	
	
	ctx->py->scripts = NULL;
#endif
    return 0;
}

int pcilib_py_add_script_dir(pcilib_t *ctx)
{
#ifdef BUILD_PYTHON_MODULES
	//create path string, where the model scripts should be
	static int model_dir_added = 0;
	if(!model_dir_added)
	{
		char* model_dir = getenv("PCILIB_MODEL_DIR");
		char* model_path = malloc(strlen(model_dir) + strlen(ctx->model) + 2);
		if (!model_path) return PCILIB_ERROR_MEMORY;
		sprintf(model_path, "%s/%s", model_dir, ctx->model);
		//add path to python
		PyObject* path = PySys_GetObject("path");
		if(PyList_Append(path, PyString_FromString(model_path)) == -1)
		{
			pcilib_error("Cant set PCILIB_MODEL_DIR library path to python.");
			free(model_path);
			return PCILIB_ERROR_FAILED;
		}
		free(model_path);
		model_dir_added = 1;
	}
#endif
	return 0;
}

void pcilib_free_py(pcilib_t *ctx) {
#ifdef BUILD_PYTHON_MODULES
	int py_initialized_inside = 0;
	
    if (ctx->py) {		
		if(ctx->py->py_initialized_inside)
			py_initialized_inside = 1;
		
        // Dict and module references are borrowed
        free(ctx->py);
        ctx->py = NULL;
    }
    
    if(py_initialized_inside)
		Py_Finalize();
#endif
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
#ifdef BUILD_PYTHON_MODULES
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
#endif

int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value) {
#ifdef BUILD_PYTHON_MODULES
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
#else
	pcilib_error("Current build not support python.");
    return PCILIB_ERROR_NOTAVAILABLE;
#endif
}

pcilib_py_object* pcilib_get_value_as_pyobject(pcilib_t* ctx, pcilib_value_t *val, int *ret)
{
#ifdef BUILD_PYTHON_MODULES	
	int err;
	
	switch(val->type)
	{
		case PCILIB_TYPE_INVALID:
            pcilib_error("Invalid register output type (PCILIB_TYPE_INVALID)");
			if (ret) *ret = PCILIB_ERROR_NOTSUPPORTED;
			return NULL;
			
		case PCILIB_TYPE_STRING:
            pcilib_error("Invalid register output type (PCILIB_TYPE_STRING)");
            if (ret) *ret = PCILIB_ERROR_NOTSUPPORTED;
			return NULL;
		
		case PCILIB_TYPE_LONG:
		{
			long ret_val;
			ret_val = pcilib_get_value_as_int(ctx, val, &err);
			
			if(err)
			{
                if (ret) *ret = err;
				return NULL;
			}
			
			if (ret) *ret = 0;
			return (PyObject*)PyInt_FromLong((long) ret_val);
		}
		
		case PCILIB_TYPE_DOUBLE:
		{
			double ret_val;
			ret_val = pcilib_get_value_as_float(ctx, val, &err);
			
			if(err)
			{
                if (ret) *ret = err;
				return NULL;
			}
			
			if (ret) *ret = 0;
			return (PyObject*)PyFloat_FromDouble((double) ret_val);
		}
		
		default:
			if (ret) *ret = PCILIB_ERROR_NOTSUPPORTED;
            pcilib_error("Invalid register output type (unknown)");
			return NULL;
	}
#else
	pcilib_error("Current build not support python.");
    if (ret) *ret = PCILIB_ERROR_NOTAVAILABLE;
	return NULL;
#endif
}

int pcilib_set_value_from_pyobject(pcilib_t* ctx, pcilib_value_t *val, pcilib_py_object* pyObjVal)
{
#ifdef BUILD_PYTHON_MODULES
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
#else
	pcilib_error("Current build not support python.");
    return PCILIB_ERROR_NOTAVAILABLE;
#endif
}

int pcilib_py_init_script(pcilib_t *ctx, const char* module_name)
{
#ifdef BUILD_PYTHON_MODULES
	//extract module name from script name
	char* py_module_name = strdup(module_name);
	py_module_name = strtok(py_module_name, ".");
	if(!py_module_name)
	{
		pcilib_error("Invalid script name specified in XML property (%s)."
					 " Seems like name doesnt contains extension", module_name);
		return PCILIB_ERROR_INVALID_DATA;
	}
	
	pcilib_script_s* module = NULL;
	HASH_FIND_STR( ctx->py->scripts, module_name, module);
	if(module)
	{
		pcilib_warning("Python module %s is already in hash. Skip init step", module_name);
		return 0;
	}
	
	//import python script
	PyObject* py_script_module = PyImport_ImportModule(py_module_name);
	if(!py_script_module)
	{
		printf("Error in import python module: ");
		PyErr_Print();
		free(py_module_name);
		return PCILIB_ERROR_INVALID_DATA;
	}
	free(py_module_name);

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
                               PyUnicode_FromString("set_pcilib"),
                               PyCObject_FromVoidPtr(ctx, NULL),
							   NULL);
	}
	
	//Success. Create struct and initialize values
	module = malloc(sizeof(pcilib_script_s));
	if (!module)
		return PCILIB_ERROR_MEMORY;
	module->module = py_script_module;
	module->name = module_name;
	HASH_ADD_STR( ctx->py->scripts, name, module);
#endif
	return 0;
}

int pcilib_py_get_transform_script_properties(pcilib_t *ctx, const char* module_name,
pcilib_access_mode_t *mode)
{
#ifdef BUILD_PYTHON_MODULES
	pcilib_script_s *module;
	
	HASH_FIND_STR(ctx->py->scripts, module_name, module);
	if(!module)
	{
		pcilib_error("Failed to find script module (%s) in hash", module_name);
		return PCILIB_ERROR_NOTFOUND;
	}
	
	PyObject* dict = PyModule_GetDict(module->module);
	//Setting correct mode
	mode[0] = 0;
	if(PyDict_Contains(dict, PyString_FromString("read_from_register")))
		mode[0] |= PCILIB_ACCESS_R;	
	if(PyDict_Contains(dict, PyString_FromString("write_to_register")))
		mode[0] |= PCILIB_ACCESS_W;	
	return 0;
#else
	mode[0] = PCILIB_ACCESS_RW;
	return 0;
#endif
}

int pcilib_py_free_script(pcilib_t *ctx, const char* module_name)
{
#ifdef BUILD_PYTHON_MODULES
	pcilib_script_s *module;
	HASH_FIND_STR(ctx->py->scripts, module_name, module);
	
	if(!module)
	{
		pcilib_warning("Cant find Python module %s in hash. Seems it has already deleted.", module_name);
		return 0;
	}
		
	if(module->module)
	{
		module->module = NULL;
	}
	
	HASH_DEL(ctx->py->scripts, module);
	free(module);
#endif
	return 0;
}

int pcilib_script_run_func(pcilib_t *ctx, const char* module_name,
                           const char* func_name,  pcilib_value_t *val)
{
#ifdef BUILD_PYTHON_MODULES
	int err;
	pcilib_script_s *module;
	HASH_FIND_STR(ctx->py->scripts, module_name, module);
	if(!module)
	{
		pcilib_error("Failed to find script module (%s) in hash", module_name);
		return PCILIB_ERROR_NOTFOUND;
	}
	
	PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *input = pcilib_get_value_as_pyobject(ctx, val, &err);
    if(err)
       return err;
       
	PyObject *py_func_name = PyUnicode_FromString(func_name);
   	PyObject *ret = PyObject_CallMethodObjArgs(module->module,
											   py_func_name,
											   input,
											   NULL);
    
    

    Py_XDECREF(py_func_name);
	Py_XDECREF(input);
	PyGILState_Release(gstate);
	
	if (!ret) 
	{
	   printf("Python script error: ");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}
	
	if(ret != Py_None)
	{
        err = pcilib_set_value_from_pyobject(ctx, val, ret);
		Py_XDECREF(ret);
	
		if(err)
		{
			pcilib_error("Failed to convert python script return value to internal type: %i", err);
			return err;
		}
	}
    return 0;
#else
	pcilib_error("Current build not support python.");
    return PCILIB_ERROR_NOTAVAILABLE;
#endif
}
