#include "config.h"

#ifdef HAVE_PYTHON
# include <Python.h>
#endif /* HAVE_PYTHON */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <alloca.h>

#include "pci.h"
#include "debug.h"
#include "pcilib.h"
#include "py.h"
#include "error.h"

#ifdef HAVE_PYTHON
typedef struct pcilib_script_s pcilib_script_t;

struct pcilib_script_s {
    const char *name;			/**< Script name */
    PyObject *module;			/**< PyModule object, contains script enviroment */	
    UT_hash_handle hh;			/**< hash */
};

struct pcilib_py_s {
    int finalyze; 			/**< Indicates, that we are initialized from wrapper and should not destroy Python resources in destructor */
    PyObject *main_module;		/**< Main interpreter */
    PyObject *global_dict;		/**< Dictionary of main interpreter */
    PyObject *pcilib_pywrap;		/**< pcilib wrapper module */
    pcilib_script_t *script_hash;	/**< Hash with loaded scripts */
};
#endif /* HAVE_PYTHON */

void pcilib_log_python_error(const char *file, int line, pcilib_log_flags_t flags, pcilib_log_priority_t prio, const char *msg, ...) {
    va_list va;
    const char *type = NULL;
    const char *val = NULL;

#ifdef HAVE_PYTHON
    PyGILState_STATE gstate;
    PyObject *pytype = NULL;
    PyObject *pyval = NULL;
    PyObject *pytraceback = NULL;


    gstate = PyGILState_Ensure();
    if (PyErr_Occurred()) {
	   PyErr_Fetch(&pytype, &pyval, &pytraceback);
	   type = PyString_AsString(pytype);
	   val = PyString_AsString(pyval);
    }
    PyGILState_Release(gstate);
#endif /* HAVE_PYTHON */
    
    va_start(va, msg);
    if (type) {
	char *str;
	size_t len = 32;

	if (msg) len += strlen(msg);
	if (type) len += strlen(type);
	if (val) len += strlen(val);
	    
	str = alloca(len * sizeof(char));
	if (str) {
	    if (msg&&val)
		sprintf(str, "%s <%s: %s>", msg, type, val);
	    else if (msg)
		sprintf(str, "%s <%s>", msg, type);
	    else if (val)
		sprintf(str, "Python error %s: %s", type, val);
	    else
		sprintf(str, "Python error %s", type);
	    
	    pcilib_log_vmessage(file, line, flags, prio, str, va);
	}
    } else  {
	pcilib_log_vmessage(file, line, flags, prio, msg, va);
    }
    va_end(va);

#ifdef HAVE_PYTHON
    if (pytype) Py_XDECREF(pytype);
    if (pyval) Py_XDECREF(pyval);
    if (pytraceback) Py_XDECREF(pytraceback);
#endif /* HAVE_PYTHON */
}



int pcilib_init_py(pcilib_t *ctx) {
#ifdef HAVE_PYTHON
    ctx->py = (pcilib_py_t*)malloc(sizeof(pcilib_py_t));
    if (!ctx->py) return PCILIB_ERROR_MEMORY;

    memset(ctx->py, 0, sizeof(pcilib_py_t));

    if(Py_IsInitialized())
	      ctx->py->finalyze = 0;
    else {
        Py_Initialize();
        
    	  // Since python is being initializing from c programm, it needs to initialize threads to work properly with c threads
        PyEval_InitThreads();
        PyEval_ReleaseLock();
    }
		
    ctx->py->main_module = PyImport_AddModule("__parser__");
    if (!ctx->py->main_module) {
	   pcilib_python_error("Error importing python parser");
      return PCILIB_ERROR_FAILED;
    }

    ctx->py->global_dict = PyModule_GetDict(ctx->py->main_module);
    if (!ctx->py->global_dict) {
      pcilib_python_error("Error locating global python dictionary");
      return PCILIB_ERROR_FAILED;
    }

    PyObject *pywrap = PyImport_ImportModule("pcipywrap");
    if (!pywrap) {
      pcilib_python_error("Error importing pcilib python wrapper");
      return PCILIB_ERROR_FAILED;
    }
	
    PyObject *mod_name = PyString_FromString("Pcipywrap");
    PyObject *ctx_py_ptr = PyCObject_FromVoidPtr(ctx, NULL);
    ctx->py->pcilib_pywrap = PyObject_CallMethodObjArgs(pywrap, mod_name, ctx_py_ptr, NULL);
    Py_DECREF(ctx_py_ptr);
    Py_DECREF(mod_name);
	
    if (!ctx->py->pcilib_pywrap) {
	   pcilib_python_error("Error initializing python wrapper");
      return PCILIB_ERROR_FAILED;
    }
#endif /* HAVE_PYTHON */
    return 0;
}

int pcilib_py_add_script_dir(pcilib_t *ctx, const char *dir) {
#ifdef HAVE_PYTHON
   PyObject* pypath;
   char *script_dir;

   const char *model_dir = getenv("PCILIB_MODEL_DIR");
   if (!model_dir) model_dir = PCILIB_MODEL_DIR;

   if (!dir) dir = ctx->model;

   if (*dir == '/') {
      script_dir = (char*)dir;
   } else {
      script_dir = alloca(strlen(model_dir) + strlen(dir) + 2);
      if (!script_dir) return PCILIB_ERROR_MEMORY;
      sprintf(script_dir, "%s/%s", model_dir, dir);
   }

   pypath = PySys_GetObject("path");
   if (!pypath) {
      pcilib_python_error("Can't get python path");
      return PCILIB_ERROR_FAILED;
   }
   
   PyObject* new_path = PyString_FromString(script_dir);

   //Check if sys.path already contains our path 
   PyObject* dict = PyDict_New();
   PyObject* cur_py = PyString_FromString("cur");
   PyObject* path_py = PyString_FromString("path");
   PyDict_SetItem(dict, cur_py, new_path);
   PyDict_SetItem(dict, path_py, pypath);
   Py_DECREF(cur_py);
   Py_DECREF(path_py);

   PyObject* pyret = PyRun_String("cur in path", Py_eval_input,
                      ctx->py->global_dict, 
                      dict);
   Py_DECREF(dict);
                      
   if(pyret == Py_False) {
      if(PyList_Append(pypath, new_path) == -1) {
         pcilib_python_error("Can't add directory (%s) to python path", script_dir);
         return PCILIB_ERROR_FAILED;
      }
   }
   else if(pyret == Py_True)
   {
      printf("1\n");
      Py_DECREF(new_path);
      return 0;
   }
   
#endif /* HAVE_PYTHON */

    return 0;
}

void pcilib_free_py(pcilib_t *ctx) {
#ifdef HAVE_PYTHON
   int finalyze = 0;
	
   if (ctx->py) {

      if(ctx->py->finalyze) finalyze = 1;
      if (ctx->py->script_hash) {
         pcilib_script_t *script, *script_tmp;

         HASH_ITER(hh, ctx->py->script_hash, script, script_tmp) {
            Py_DECREF(script->module);
            
            HASH_DEL(ctx->py->script_hash, script);
            free(script);
         }
         ctx->py->script_hash = NULL;
      }
      
      Py_DECREF(ctx->py->pcilib_pywrap);

      free(ctx->py);
      ctx->py = NULL;
   }
    
    if (finalyze)
	   Py_Finalize();
#endif /* HAVE_PYTHON */
}

int pcilib_py_load_script(pcilib_t *ctx, const char *script_name) {
#ifdef HAVE_PYTHON
   PyObject* pymodule;
   pcilib_script_t *module = NULL;


   char *module_name = strdupa(script_name);
   if (!module_name) return PCILIB_ERROR_MEMORY;

   char *py = strrchr(module_name, '.');
   if ((!py)||(strcasecmp(py, ".py"))) {
      pcilib_error("Invalid script name (%s) is specified", script_name);
      return PCILIB_ERROR_INVALID_ARGUMENT;
   }
   *py = 0;

   HASH_FIND_STR(ctx->py->script_hash, script_name, module);
   if (module) return 0;

   pymodule = PyImport_ImportModule(module_name);
   if (!pymodule) {
      pcilib_python_error("Error importing script (%s)", script_name);
      return PCILIB_ERROR_FAILED;
   }

   module = (pcilib_script_t*)malloc(sizeof(pcilib_script_t));
   if (!module) return PCILIB_ERROR_MEMORY;

   module->module = pymodule;
   module->name = script_name;
   HASH_ADD_KEYPTR(hh, ctx->py->script_hash, module->name, strlen(module->name), module);
#endif /* HAVE_PYTHON */
    return 0;
}

int pcilib_py_get_transform_script_properties(pcilib_t *ctx, const char *script_name, pcilib_access_mode_t *mode_ret) {
    pcilib_access_mode_t mode = 0;

#ifdef HAVE_PYTHON
   PyObject *dict;
   PyObject *pystr;
   pcilib_script_t *module;

   HASH_FIND_STR(ctx->py->script_hash, script_name, module);

   if(!module) {
      pcilib_error("Script (%s) is not loaded yet", script_name);
      return PCILIB_ERROR_NOTFOUND;
   }

   dict = PyModule_GetDict(module->module);
   if (!dict) {
      pcilib_python_error("Error getting dictionary for script (%s)", script_name);
      return PCILIB_ERROR_FAILED;
   }

   pystr = PyString_FromString("read_from_register");
   if (pystr) {
      if (PyDict_Contains(dict, pystr)) mode |= PCILIB_ACCESS_R;
      Py_XDECREF(pystr);
   }

   pystr = PyString_FromString("write_to_register");
   if (pystr) {
      if (PyDict_Contains(dict, pystr)) mode |= PCILIB_ACCESS_W;
      Py_XDECREF(pystr);
   }
#endif /* HAVE_PYTHON */

    if (mode_ret) *mode_ret = mode;
    return 0;
}

pcilib_py_object *pcilib_get_value_as_pyobject(pcilib_t* ctx, pcilib_value_t *val, int *ret) {
#ifdef HAVE_PYTHON	
    int err = 0;
    PyObject *res = NULL;
    PyGILState_STATE gstate;

    long ival;
    double fval;
	
   gstate = PyGILState_Ensure();
   switch(val->type) {
      case PCILIB_TYPE_LONG:
         ival = pcilib_get_value_as_int(ctx, val, &err);
         if (!err) res = (PyObject*)PyInt_FromLong(ival);
         break;
      case PCILIB_TYPE_DOUBLE:
         fval = pcilib_get_value_as_float(ctx, val, &err);
         if (!err) res = (PyObject*)PyFloat_FromDouble(fval);
         break;	
      default:
         PyGILState_Release(gstate);
         pcilib_error("Can't convert pcilib value of type (%lu) to PyObject", val->type);
         if (ret) *ret = PCILIB_ERROR_NOTSUPPORTED;
         return NULL;
   }
   PyGILState_Release(gstate);

   if (err) {
      if (ret) *ret = err;
      return NULL;
   } else if (!res) {
      if (ret) *ret = PCILIB_ERROR_MEMORY;
      return res;
   } 

    if (ret) *ret = 0;
    return res;
#else /* HAVE_PYTHON */
    pcilib_error("Python is not supported");
    return NULL;
#endif /* HAVE_PYTHON */
}

int pcilib_set_value_from_pyobject(pcilib_t* ctx, pcilib_value_t *val, pcilib_py_object *pval) {
#ifdef HAVE_PYTHON
    int err = 0;
    PyObject *pyval = (PyObject*)pval;
    PyGILState_STATE gstate;
	
    gstate = PyGILState_Ensure();
    if (PyInt_Check(pyval)) {
        err = pcilib_set_value_from_int(ctx, val, PyInt_AsLong(pyval));
    } else if (PyFloat_Check(pyval)) {
        err = pcilib_set_value_from_float(ctx, val, PyFloat_AsDouble(pyval));
    } else if (PyString_Check(pyval)) {
        err = pcilib_set_value_from_string(ctx, val, PyString_AsString(pyval));
    } else {
        PyGILState_Release(gstate);
        pcilib_error("Can't convert PyObject to polymorphic pcilib value");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    PyGILState_Release(gstate);

    return err;
#else /* HAVE_PYTHON */
    pcilib_error("Python is not supported");
    return PCILIB_ERROR_NOTSUPPORTED;
#endif /* HAVE_PYTHON */
}

#ifdef HAVE_PYTHON
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
#endif /* HAVE_PYTHON */

int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *value) {
#ifdef HAVE_PYTHON
    int err;
      
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
     
    free(code);

    if (!obj) {
        pcilib_error("Failed to run the Python code: %s", code);
        return PCILIB_ERROR_FAILED;
    }

    pcilib_debug(VIEWS, "Evaluating a Python string \'%s\' to %lf=\'%s\'", codestr, PyFloat_AsDouble(obj), code);
    err = pcilib_set_value_from_float(ctx, value, PyFloat_AsDouble(obj));
    Py_DECREF(obj);

    return err;
#else /* HAVE_PYTHON */
    pcilib_error("Current build not support python.");
    return PCILIB_ERROR_NOTAVAILABLE;
#endif /* HAVE_PYTHON */
}

int pcilib_py_eval_func(pcilib_t *ctx, const char *script_name, const char *func_name, pcilib_value_t *val) {
#ifdef HAVE_PYTHON
    int err = 0;
    PyObject *pyfunc;
    PyObject *pyval = NULL, *pyret;
    pcilib_script_t *module = NULL;

    HASH_FIND_STR(ctx->py->script_hash, script_name, module);

    if (!module) {
	   pcilib_error("Script (%s) is not loaded", script_name);
	   return PCILIB_ERROR_NOTFOUND;
    }

    if (val) {
	   pyval = pcilib_get_value_as_pyobject(ctx, val, &err);
	   if (err) return err;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    pyfunc = PyUnicode_FromString(func_name);
    if (!pyfunc) {
	   if (pyval) Py_XDECREF(pyval);
	   PyGILState_Release(gstate);
	   return PCILIB_ERROR_MEMORY;
    }

    pyret = PyObject_CallMethodObjArgs(module->module, pyfunc, ctx->py->pcilib_pywrap, pyval, NULL);

    Py_XDECREF(pyfunc);
    Py_XDECREF(pyval);
	
    if (!pyret) {
	   PyGILState_Release(gstate);
	   pcilib_python_error("Error executing function (%s) of python script (%s)", func_name, script_name);
	   return PCILIB_ERROR_FAILED;
    }

    if ((val)&&(pyret != Py_None))
	   err = pcilib_set_value_from_pyobject(ctx, val, pyret);

    Py_XDECREF(pyret);
    PyGILState_Release(gstate);

    return err;
#else /* HAVE_PYTHON */
    pcilib_error("Python is not supported");
    return PCILIB_ERROR_NOTSUPPORTED;
#endif /* HAVE_PYTHON */
}
