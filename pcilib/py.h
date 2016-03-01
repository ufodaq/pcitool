#ifndef _PCILIB_PY_H
#define _PCILIB_PY_H

#include <pcilib.h>
#include <pcilib/error.h>

#define pcilib_python_error(...)	pcilib_log_python_error(__FILE__, __LINE__, PCILIB_LOG_DEFAULT, PCILIB_LOG_ERROR, __VA_ARGS__)

typedef struct pcilib_py_s pcilib_py_t;
typedef void pcilib_py_object;

#ifdef __cplusplus
extern "C" {
#endif

void pcilib_log_python_error(const char *file, int line, pcilib_log_flags_t flags, pcilib_log_priority_t prio, const char *msg, ...);

/** Initializes Python engine 
 *
 * This function will return success if Python support is disabled. Only functions
 * executing python call, like pcilib_py_eval_string(), return errors. Either way,
 * no script directories are configured. The pcilib_add_script_dir() call is used
 * for this purpose.
 * 
 * @param[in,out] ctx 	- pcilib context
 * @return 		- error or 0 on success
 */
int pcilib_init_py(pcilib_t *ctx);

/** Cleans up memory used by various python structures 
 * and finalyzes python environment if pcilib is not started from python script
 *
 * @param[in] ctx 	- the pcilib_t context
 */
void pcilib_free_py(pcilib_t *ctx);

/** Add an additional path to look for python scripts
 *
 * The default location for python files is /usr/local/share/pcilib/models/@b{model}.
 * This can be altered using CMake PCILIB_MODEL_DIR variable while building or using 
 * PCILIB_MODEL_DIR environmental variable dynamicly. The default location is added
 * with @b{location} = NULL. Additional directories can be added as well either 
 * by specifying relative path from the default directory or absolute path in the 
 * system.
 *
 * @param[in,out] ctx 	- pcilib context
 * @param[in] location 	- NULL or path to additional scripts
 * @return 		- error or 0 on success
 */
int pcilib_py_add_script_dir(pcilib_t *ctx, const char *location);

/** Loads the specified python script
 *
 * Once loaded the script is available until pcilib context is destryoed.
 * 
 * @param[in,out] ctx 	- pcilib context
 * @param[in] name	- script name, the passed variable is referenced and, hence, should have static duration
 * @return 		- error or 0 on success
 */
int pcilib_py_load_script(pcilib_t *ctx, const char *name);

/** Check if the specified script can be used as transform view and detects transform configuration
 *
 * @param[in,out] ctx 	- pcilib context
 * @param[in] name	- script name
 * @param[out] mode	- supported access mode (read/write/read-write)
 * @return 		- error or 0 on success
 */
int pcilib_py_get_transform_script_properties(pcilib_t *ctx, const char *name, pcilib_access_mode_t *mode);

/**
 * Get the PyObject from the polymorphic type. The returned value should be cleaned with Py_XDECREF()
 * @param[in] ctx	- pcilib context
 * @param[in] val	- initialized polymorphic value of arbitrary type
 * @param[out] err	- error code or 0 on sccuess
 * @return		- valid PyObject or NULL in the case of error
 */
pcilib_py_object *pcilib_get_value_as_pyobject(pcilib_t* ctx, pcilib_value_t *val, int *err);

/**
 * Initializes the polymorphic value from PyObject. If `val` already contains the value, cleans it first. 
 * Therefore, before first usage the value should be always initialized to 0.
 * @param[in] ctx       - pcilib context
 * @param[in,out] val   - initialized polymorphic value
 * @param[in] pyval     - valid PyObject* containing PyInt, PyFloat, or PyString
 * @return              - 0 on success or memory error
 */
int pcilib_set_value_from_pyobject(pcilib_t* ctx, pcilib_value_t *val, pcilib_py_object *pyval);

/** Evaluates the specified python code and returns result in @b{val}
 *
 * The python code may include special variables which will be substituted by pcitool before executing Python interpreter
 * @b{$value} 		- will be replaced by the current value of the @b{val} parameter
 * @b{$reg}		- will be replaced by the current value of the specified register @b{reg}
 * @b{${/prop/temp}}	- will be replaced by the current value of the specified property @b{/prop/temp} 
 * @b{${/prop/temp:C}}	- will be replaced by the current value of the specified property @b{/prop/temp} in the given units

 * @param[in,out] ctx 	- pcilib context
 * @param[in] codestr	- python code to evaluate
 * @param[in,out] val	- Should contain the value which will be substituted in place of @b{$value} and on 
 * 			successful execution will contain the computed value
 * @return 		- error or 0 on success
 */
int pcilib_py_eval_string(pcilib_t *ctx, const char *codestr, pcilib_value_t *val);

/** Execute the specified function in the Python script which was loaded with pcilib_py_load_script() call
 *
 * The function is expected to accept two paramters. The first parameter is pcipywrap context and the @b{val}
 * is passed as the second parameter. The return value of the script will be returned in the @b{val} as well.
 * If function returns Py_None, the value of @b{val} will be unchanged.
 *
 * @param[in,out] ctx 	- pcilib context
 * @param[in] script	- script name
 * @param[in] func	- function name
 * @param[in,out] val	- Should contain the value of second parameter of the function before call and on 
 * 			successful return will contain the returned value
 * @return 		- error or 0 on success
 */
int pcilib_py_eval_func(pcilib_t *ctx, const char *script, const char *func, pcilib_value_t *val);


/** Clone pcilib_py_t content without scripts hash
 *  @param[in] in	- pcilib_py_t content to clone
 *  @param[out] err - error
 *  @return 		- NULL or cloned pcilib_py_t content pointer on success
 */
pcilib_py_t* pcilib_init_py_ctx(pcilib_py_t* in, int *err);

/**
 * @brief pcilib_t independent variant  pcilib_free_py
 * @param ctx_py[in,out] - pcilib_py_t context
 */
void pcilib_free_py_ctx(pcilib_py_t *ctx_py);

/** pcilib_t independent variant of pcilib_py_eval_func()
 * @param ctx_py[in,out] - pcilib_py_t context
 * @param name[in] - script name
 * @param name[in] - function name
 * @param pyval[in] - input value (will be decref in this fucntion)
 * @param err[out] - error
 * @return value returned by python function
 */
pcilib_py_object* pcilib_py_ctx_eval_func(pcilib_py_t *ctx_py,
                                          const char *name,
                                          const char *func_name, 
                                          pcilib_py_object *pyval, 
                                          int *err);

/**
 * @brief pcilib_t independent variant of pcilib_py_add_script_dir
 * @param ctx_py[in,out] - pcilib_py_t context
 * @param[in] location 	- NULL or path to additional scripts
 * @return
 */
int pcilib_py_ctx_add_script_dir(pcilib_py_t *ctx_py, const char *location);

/**
 * @brief pcilib_t independent variant of pcilib_py_load_script
 * @param ctx_py[in,out] - pcilib_py_t context
 * @param[in] name	- script name, the passed variable is referenced and, hence, should have static duration
 * @return
 */
int pcilib_py_ctx_load_script(pcilib_py_t *ctx_py, const char *name);

/**
 * @brief Returns information about scripts aviable in model
 * @param ctx_py[in,out] - pcilib_py_t context
 * @return List with information about scripts
 */
pcilib_py_object *pcilib_py_ctx_get_scripts_info(pcilib_py_t *ctx_py);

/** Wrap for PyDict_SetItem, with decrease reference counting after set.
 */
void pcilib_pydict_set_item(pcilib_py_object* dict, pcilib_py_object* name, pcilib_py_object* value);

/** Wrap for PyList_Append, with decrease reference counting after append.
 */
void pcilib_pylist_append(pcilib_py_object* list, pcilib_py_object* value);
#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_PY_H */
