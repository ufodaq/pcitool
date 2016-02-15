#include "pci.h"
#include "error.h"
#include <Python.h>

/*!
 * \brief Global pointer to pcilib_t context.
 * Used by set_pcilib and read_register.
 */
pcilib_t* __ctx = 0;

char* full_log = NULL;

/*!
 * \brief Wraping for vsnprintf function, that saves string to char*
 * \return saved from vsnprintf string
 */
char* vmake_str(const char* msg, va_list vl)
{
	char *buf;
	size_t sz;
	
	va_list vl_copy;
	va_copy(vl_copy, vl);
	
	sz = vsnprintf(NULL, 0, msg, vl);
	buf = (char *)malloc(sz + 1);
	
	if(!buf)
	{
		return NULL;
	}

	vsnprintf(buf, sz+1, msg, vl_copy);
	va_end(vl_copy);
	
	return buf;
}

/*!
 * \brief Wraping for vsnprintf function, that saves string to char*
 * \return saved from vsnprintf string
 */
char* make_str(const char* msg, ...)
{
	va_list vl;
    va_start(vl, msg);
	char *buf = vmake_str(msg, vl);
	va_end(vl);
	return buf;
}

/*!
 * \brief Version of pcilib_logger_t, that saves error text to Python exeption
 */
void pcilib_print_error_to_py(void *arg, const char *file, int line, 
							  pcilib_log_priority_t prio, const char *msg,
							  va_list va) {							  
	//wrap error message with file and line number
	char* buf_raw_msg = vmake_str(msg, va);
	char* buf_wrapped_message = make_str("%s [%s:%d]\n", buf_raw_msg, file, line);
	
	if(prio == PCILIB_LOG_ERROR)
	{
		if(!full_log)
			full_log = make_str("");
    
		if(strlen(buf_wrapped_message) >= 2 &&
		   buf_wrapped_message[0] == '#' &&
		   buf_wrapped_message[1] == 'E')
		{
			char* wrapped_exeption = make_str("%sprogramm error log:\n%s", &(buf_wrapped_message[2]), full_log);
			free(full_log);
			full_log = NULL;
			
			PyErr_SetString(PyExc_Exception, wrapped_exeption);
			free(wrapped_exeption);
		}
		else
		{
			//copy received message to log
			char* buf = full_log;
			full_log = make_str("%s%s", buf, buf_wrapped_message);
			free(buf);
		}
	}
	else
		printf(buf_wrapped_message);
		
	free(buf_wrapped_message);
	free(buf_raw_msg);
}

/*!
 * \brief Redirect pcilib standart log stream to exeption text.
 * Logger will accumulate errors untill get message, starts with "#E".
 * After that, logger will write last error, and all accumulated errors
 * to Python exeption text
 */
void __redirect_logs_to_exeption()
{
	pcilib_set_logger(pcilib_get_logger_min_prio(), 
					  pcilib_print_error_to_py,
					  pcilib_get_logger_argument());
}

/*!
 * Destructor for pcilib_t
 */
void close_pcilib_instance(void *ctx)
{
	if(ctx != __ctx)
		pcilib_close(ctx);
}

/*!
 * \brief Wraps for pcilib_open function.
 * \param[in] fpga_device path to the device file [/dev/fpga0]
 * \param[in] model specifies the model of hardware, autodetected if NULL is passed
 * \return Pointer to pcilib_t, created by pcilib_open; NULL with exeption text, if failed.
 */
PyObject* create_pcilib_instance(const char *fpga_device, const char *model)
{
	//opening device
    pcilib_t* ctx = pcilib_open(fpga_device, model);
    if(!ctx)
    {
		pcilib_error("#E Failed pcilib_open(%s, %s)", fpga_device, model);
		return NULL;
	}
	
	return PyCObject_FromVoidPtr((void*)ctx, close_pcilib_instance);
}

/*!
 * \brief Closes current pciliv instance, if its open.
 */
void close_curr_pcilib_instance()
{
    if(__ctx)
    {
        pcilib_close(__ctx);
        __ctx = NULL;
    }
}

/*!
 * \brief Returns current opened pcilib_t instatnce
 * \return Pointer to pcilib_t, serialized to bytearray
 */
PyObject* get_curr_pcilib_instance()
{
    return PyByteArray_FromStringAndSize((const char*)&__ctx, sizeof(pcilib_t*));
}

/*!
 * \brief Sets pcilib context to wraper.
 * \param[in] addr Pointer to pcilib_t, serialized to PyCObject
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* set_pcilib(PyObject* addr)
{
	if(!PyCObject_Check(addr))
	{
        pcilib_error("#E Incorrect addr type. Only PyCObject is allowed");
		return NULL;
	}
	
	__ctx = (pcilib_t*)PyCObject_AsVoidPtr(addr);
	
	return PyInt_FromLong((long)1);
}


/*!
 * \brief Reads register value. Wrap for pcilib_read_register function.
 * \param[in] regname the name of the register
 * \param[in] bank should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * \return register value, can be integer or float type; NULL with exeption text, if failed.
 */
PyObject* read_register(const char *regname, const char *bank)
{
	if(!__ctx)
	{
        pcilib_error("#E pcilib_t handler not initialized");
		return NULL;
	}

	pcilib_value_t val = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
	err = pcilib_read_register(__ctx, bank, regname, &reg_value);
	if(err)
	{
		pcilib_error("#E Failed pcilib_read_register");
		return NULL;
	}
	
	err = pcilib_set_value_from_register_value(__ctx, &val, reg_value);
	if(err)
	{
		pcilib_error("#E Failed pcilib_set_value_from_register_value");
		return NULL;
	}

    return pcilib_get_value_as_pyobject(__ctx, &val);
}

/*!
 * \brief Writes value to register. Wrap for pcilib_write_register function.
 * \param[in] val Register value, that needs to be set. Can be int, float or string.
 * \param[in] regname the name of the register
 * \param[in] bank should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* write_register(PyObject* val, const char *regname, const char *bank)
{
	if(!__ctx)
	{
        pcilib_error("pcilib_t handler not initialized");
		return NULL;
	}
	
	
	pcilib_value_t val_internal = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
	err = pcilib_set_value_from_pyobject(__ctx, val, &val_internal);
	
	if(err)
	{
		pcilib_error("#E Failed pcilib_set_value_from_pyobject");
		return NULL;
	}
	
	
	reg_value = pcilib_get_value_as_register_value(__ctx, &val_internal, &err);
	if(err)
	{
		pcilib_error("#E Failed pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
	err = pcilib_write_register(__ctx, bank, regname, reg_value);
	
	if(err)
	{
		pcilib_error("#E Failed pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
    return PyInt_FromLong((long)1);
}

/*!
 * \brief Reads propety value. Wrap for pcilib_get_property function.
 * \param[in] prop property name (full name including path)
 * \return property value, can be integer or float type; NULL with exeption text, if failed.
 */
PyObject* get_property(const char *prop)
{
	if(!__ctx)
	{
        pcilib_error("#E pcilib_t handler not initialized");
		return NULL;
	}
	
	int err;
	pcilib_value_t val = {0};
	
	err  = pcilib_get_property(__ctx, prop, &val);
	
	if(err)
	{
		pcilib_error("#E Failed pcilib_get_property, (error %i)", err);
		return NULL;
	}
	
    return pcilib_get_value_as_pyobject(__ctx, &val);
}

/*!
 * \brief Writes value to property. Wrap for pcilib_set_property function.
 * \param[in] prop property name (full name including path)
 * \param[in] val Property value, that needs to be set. Can be int, float or string.
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* set_property(PyObject* val, const char *prop)
{
	int err;
	
	if(!__ctx)
	{
        pcilib_error("#E pcilib_t handler not initialized");
		return NULL;
	}
	
	pcilib_value_t val_internal = {0};
	err = pcilib_set_value_from_pyobject(__ctx, val, &val_internal);
	if(err)
	{
		pcilib_error("#E pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
	err  = pcilib_set_property(__ctx, prop, &val_internal);
	if(err)
	{
		pcilib_error("#E pcilib_set_property, (error %i)", err);
		return NULL;
	}
	
	return PyInt_FromLong((long)1);
}

/*!
 * \brief Wrap for PyDict_SetItem, with decrease reference counting after set.
 */
void pcilib_pydict_set_item(PyObject* dict, PyObject* name, PyObject* value)
{
    PyDict_SetItem(dict,
                   name,
                   value);
    Py_XDECREF(name);
    Py_XDECREF(value);
}

/*!
 * \brief Wrap for PyList_Append, with decrease reference counting after append.
 */
void pcilib_pylist_append(PyObject* list, PyObject* value)
{
    PyList_Append(list, value);
    Py_XDECREF(value);
}

void add_pcilib_value_to_dict(PyObject* dict, pcilib_value_t* val, const char *name)
{
    PyObject *py_val = (PyObject*)pcilib_get_value_as_pyobject(__ctx, val);

	if(py_val)
        pcilib_pydict_set_item(dict,
                              PyString_FromString(name),
                              py_val);
	else
        pcilib_pydict_set_item(dict,
                              PyString_FromString("defvalue"),
                              PyString_FromString("invalid"));
}

PyObject * pcilib_convert_property_info_to_pyobject(pcilib_property_info_t listItem)
{
    PyObject* pylistItem = PyDict_New();

    if(listItem.name)
        pcilib_pydict_set_item(pylistItem,
                   PyString_FromString("name"),
                   PyString_FromString(listItem.name));

    if(listItem.description)
        pcilib_pydict_set_item(pylistItem,
                   PyString_FromString("description"),
                   PyString_FromString(listItem.description));

   if(listItem.path)
        pcilib_pydict_set_item(pylistItem,
                   PyString_FromString("path"),
                   PyString_FromString(listItem.path));

    //serialize types
    const char* type = "invalid";
    switch(listItem.type)
    {
        case PCILIB_TYPE_INVALID:
            type = "invalid";
            break;
        case PCILIB_TYPE_STRING:
             type = "string";
             break;
        case PCILIB_TYPE_DOUBLE:
             type = "double";
             break;
        case PCILIB_TYPE_LONG :
             type = "long";
             break;
        default:
             break;
    }
    pcilib_pydict_set_item(pylistItem,
               PyString_FromString("type"),
               PyString_FromString(type));


    //serialize modes
    PyObject* modes = PyList_New(0);

    if((listItem.mode & PCILIB_ACCESS_R ) == PCILIB_REGISTER_R)
        pcilib_pylist_append(modes, PyString_FromString("R"));
    if((listItem.mode & PCILIB_ACCESS_W ) == PCILIB_REGISTER_W)
        pcilib_pylist_append(modes, PyString_FromString("W"));
    if((listItem.mode & PCILIB_ACCESS_RW ) == PCILIB_REGISTER_RW)
        pcilib_pylist_append(modes, PyString_FromString("RW"));
    if((listItem.mode & PCILIB_REGISTER_INCONSISTENT) == PCILIB_REGISTER_INCONSISTENT)
        pcilib_pylist_append(modes, PyString_FromString("NO_CHK"));

    pcilib_pydict_set_item(pylistItem,
                   PyString_FromString("mode"),
                   modes);

    //serialize flags
    PyObject* flags = PyList_New(0);

    if((listItem.flags & PCILIB_LIST_FLAG_CHILDS ) == PCILIB_LIST_FLAG_CHILDS)
        pcilib_pylist_append(flags, PyString_FromString("childs"));

    pcilib_pydict_set_item(pylistItem,
                   PyString_FromString("flags"),
                   flags);

    if(listItem.unit)
         pcilib_pydict_set_item(pylistItem,
                    PyString_FromString("unit"),
                    PyString_FromString(listItem.unit));

    return pylistItem;
}

PyObject * pcilib_convert_register_info_to_pyobject(pcilib_register_info_t listItem)
{
    PyObject* pylistItem = PyDict_New();

    if(listItem.name)
        pcilib_pydict_set_item(pylistItem,
		                      PyString_FromString("name"),
		                      PyString_FromString(listItem.name));

    if(listItem.description)
        pcilib_pydict_set_item(pylistItem,
                              PyString_FromString("description"),
                              PyString_FromString(listItem.description));

   if(listItem.bank)
        pcilib_pydict_set_item(pylistItem,
                              PyString_FromString("bank"),
                              PyString_FromString(listItem.bank));

    //serialize modes
    PyObject* modes = PyList_New(0);

    if((listItem.mode & PCILIB_REGISTER_R) == PCILIB_REGISTER_R)
        pcilib_pylist_append(modes, PyString_FromString("R"));
    if((listItem.mode & PCILIB_REGISTER_W) == PCILIB_REGISTER_W)
        pcilib_pylist_append(modes, PyString_FromString("W"));
    if((listItem.mode & PCILIB_REGISTER_RW) == PCILIB_REGISTER_RW)
        pcilib_pylist_append(modes, PyString_FromString("RW"));
    if((listItem.mode & PCILIB_REGISTER_W1C) == PCILIB_REGISTER_W1C)
        pcilib_pylist_append(modes, PyString_FromString("W1C"));
    if((listItem.mode & PCILIB_REGISTER_RW1C) == PCILIB_REGISTER_RW1C)
        pcilib_pylist_append(modes, PyString_FromString("RW1C"));
    if((listItem.mode & PCILIB_REGISTER_W1I) == PCILIB_REGISTER_W1I)
        pcilib_pylist_append(modes, PyString_FromString("W1I"));
    if((listItem.mode & PCILIB_REGISTER_RW1I) == PCILIB_REGISTER_RW1I)
        pcilib_pylist_append(modes, PyString_FromString("RW1I"));
    if((listItem.mode & PCILIB_REGISTER_INCONSISTENT) == PCILIB_REGISTER_INCONSISTENT)
        pcilib_pylist_append(modes, PyString_FromString("NO_CHK"));

    pcilib_pydict_set_item(pylistItem,
						  PyString_FromString("mode"),
                          modes);
                  
    pcilib_value_t defval = {0};
    pcilib_set_value_from_register_value(__ctx, &defval, listItem.defvalue);
    add_pcilib_value_to_dict(pylistItem, &defval, "defvalue");

    if(listItem.range)
    {
		pcilib_value_t minval = {0};
		pcilib_set_value_from_register_value(__ctx, &minval, listItem.range->min);
		
		pcilib_value_t maxval = {0};
		pcilib_set_value_from_register_value(__ctx, &maxval, listItem.range->max);
		
        PyObject* range = PyDict_New();
        add_pcilib_value_to_dict(range, &minval, "min");
        add_pcilib_value_to_dict(range, &maxval, "max");
        pcilib_pydict_set_item(pylistItem,
                              PyString_FromString("range"),
                              range);
    }

    if(listItem.values)
    {
        PyObject* values = PyList_New(0);

        for (int j = 0; listItem.values[j].name; j++)
        {
            PyObject* valuesItem = PyDict_New();
            
            pcilib_value_t val = {0};
			pcilib_set_value_from_register_value(__ctx, &val, listItem.values[j].value);

			pcilib_value_t min = {0};
			pcilib_set_value_from_register_value(__ctx, &min, listItem.values[j].min);
		
			pcilib_value_t max = {0};
			pcilib_set_value_from_register_value(__ctx, &max, listItem.values[j].max);
            
            add_pcilib_value_to_dict(valuesItem, &val, "value");
            add_pcilib_value_to_dict(valuesItem, &min, "min");
            add_pcilib_value_to_dict(valuesItem, &max, "max");

            if(listItem.values[j].name)
                pcilib_pydict_set_item(valuesItem,
									  PyString_FromString("name"),
									  PyString_FromString(listItem.values[j].name));

            if(listItem.values[j].description)
                pcilib_pydict_set_item(valuesItem,
									  PyString_FromString("name"),
									  PyString_FromString(listItem.values[j].description));

            pcilib_pylist_append(values, valuesItem);
        }

        pcilib_pydict_set_item(pylistItem,
                              PyString_FromString("values"),
                              values);
    }

    return pylistItem;
}

PyObject* get_registers_list(const char *bank)
{
	if(!__ctx)
	{
        pcilib_error("#E pcilib_t handler not initialized");
		return NULL;
	}
	
	pcilib_register_info_t *list = pcilib_get_register_list(__ctx, bank, PCILIB_LIST_FLAGS_DEFAULT);
	
	PyObject* pyList = PyList_New(0);
    for(int i = 0; i < __ctx->num_reg; i++)
    {
		//serialize item attributes
        PyObject* pylistItem = pcilib_convert_register_info_to_pyobject(list[i]);
        pcilib_pylist_append(pyList, pylistItem);
    }
    
    pcilib_free_register_info(__ctx, list);
	
	return pyList;
}

PyObject* get_register_info(const char* reg,const char *bank)
{
    if(!__ctx)
    {
        pcilib_error("#E pcilib_t handler not initialized");
        return NULL;
    }

    pcilib_register_info_t *info = pcilib_get_register_info(__ctx, bank, reg, PCILIB_LIST_FLAGS_DEFAULT);

	if(!info)
	{
        return NULL;
	}

    PyObject* py_info = pcilib_convert_register_info_to_pyobject(info[0]);

    pcilib_free_register_info(__ctx, info);

    return py_info;
}

PyObject* get_property_list(const char* branch)
{
    if(!__ctx)
    {
        pcilib_error("#E pcilib_t handler not initialized");
        return NULL;
    }

    pcilib_property_info_t *list = pcilib_get_property_list(__ctx, branch, PCILIB_LIST_FLAGS_DEFAULT);

    PyObject* pyList = PyList_New(0);

    for(int i = 0; list[i].path; i++)
    {
        //serialize item attributes
        PyObject* pylistItem = pcilib_convert_property_info_to_pyobject(list[i]);
        pcilib_pylist_append(pyList, pylistItem);
    }

    pcilib_free_property_info(__ctx, list);

    return pyList;
}
