#define _PCILIB_VIEW_TRANSFORM_C

#include <stdio.h>
#include <stdlib.h>

#include "pci.h"
#include "pcipywrap.h"
#include "error.h"

#include "model.h"
#include "script.h"

static int pcilib_script_view_module_init(pcilib_t *ctx, pcilib_script_view_description_t *v)
{
	int err;
	
	//Initialize python script, if it has not initialized already.
	if(!v->py_script_module)
	{
		if(!v->script_name)
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
			pcipywrap_path = malloc(strlen(app_dir) + strlen("/pcilib"));
			if (!pcipywrap_path) return PCILIB_ERROR_MEMORY;
			sprintf(pcipywrap_path, "%s/%s", "/pcilib", ctx->model);
		}
		else
		{
			pcipywrap_path = malloc(strlen("./pcilib"));
			if (!pcipywrap_path) return PCILIB_ERROR_MEMORY;
			sprintf(pcipywrap_path, "%s", "./pcilib");
	
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
		char* py_module_name = strtok(v->script_name, ".");
		
		if(!py_module_name)
		{
			pcilib_error("Invalid script name specified in XML property (%s)."
						 " Seems like name doesnt contains extension", v->script_name);
			return PCILIB_ERROR_INVALID_DATA;
		}
		
		//import python script
		v->py_script_module = PyImport_ImportModule(py_module_name);
		
		if(!v->py_script_module)
		{
			printf("Error in import python module: ");
			PyErr_Print();
			return PCILIB_ERROR_INVALID_DATA;
		}
	}
	
	//Initializing pcipywrap module if script use it
	PyObject* dict = PyModule_GetDict(v->py_script_module);
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
                               PyUnicode_FromString("__setPcilib"),
                               PyByteArray_FromStringAndSize((const char*)&ctx, sizeof(pcilib_t*)),
                               NULL);
	}
	
	return 0;
}

static int pcilib_script_view_read(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t regval, pcilib_value_t *val) {
	
	int err;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_script_view_description_t *v = (pcilib_script_view_description_t*)(model_info->views[view_ctx->view]);
	
	err = pcilib_script_view_module_init(ctx, v);
	if(err)
		return err;
	
	PyObject *ret = PyObject_CallMethod(v->py_script_module, "read_from_register", "()");
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

static int pcilib_script_view_write(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t *regval, pcilib_value_t *val) {
	
	int err;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_script_view_description_t *v = (pcilib_script_view_description_t*)(model_info->views[view_ctx->view]);
	
	err = pcilib_script_view_module_init(ctx, v);
	if(err)
		return err;
		
	PyObject *input = pcilib_convert_val_to_pyobject(ctx, val, printf);
	if(!input)
	{
	   printf("Failed to convert input value to Python object");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}
	
	PyObject *ret = PyObject_CallMethodObjArgs(v->py_script_module,
											   PyUnicode_FromString("write_to_register"),
											   input,
											   NULL);
	if (!ret) 
	{
	   printf("Python script error: ");
	   PyErr_Print();
	   return PCILIB_ERROR_FAILED;
	}
	
	//convert output value back to pcilib_value_t
	//no need because it wont be used later, and the script return could be none
	/*
	err = pcilib_convert_pyobject_to_val(ctx, ret, val);
	if(err)
	{
		pcilib_error("failed to convert script write_to_register function return value to internal type: %i", err);
		return err;
	}
	
	uint64_t output = pcilib_get_value_as_register_value(ctx, val, &err);
	if(err)
	{
		pcilib_error("failed to convert value to register value (%i)", err);
		return err;
	}
	regval[0] = output;
	*/

    return 0;
}

void pcilib_script_view_free_description (pcilib_t *ctx, pcilib_view_description_t *view)
{
	pcilib_script_view_description_t *v = (pcilib_script_view_description_t*)(view);
	
	if(v->script_name)
	{
		free(v->script_name);
		v->script_name = NULL;
	}
	
	if(v->py_script_module)
	{
		PyObject_Free(v->py_script_module);
		v->py_script_module = NULL;
	}
}

const pcilib_view_api_description_t pcilib_script_view_api =
  { PCILIB_VERSION, sizeof(pcilib_script_view_description_t), NULL, NULL, pcilib_script_view_free_description, pcilib_script_view_read, pcilib_script_view_write};
