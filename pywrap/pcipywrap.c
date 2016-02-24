#include "pcipywrap.h"
#include "locking.h"

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
			
		//copy received message to log
		char* buf = full_log;
		full_log = make_str("%s%s", buf, buf_wrapped_message);
		free(buf);
	}
	else
		printf(buf_wrapped_message);
		
	free(buf_wrapped_message);
	free(buf_raw_msg);
}

void set_python_exception(const char* msg, ...)
{
	va_list vl;
    va_start(vl, msg);
	char *buf = vmake_str(msg, vl);
	
	char* wrapped_exeption;
	if(full_log)
		wrapped_exeption = make_str("%s\nprogramm error log:\n%s", buf, full_log);
	else
		wrapped_exeption = buf;
	
	free(full_log);
	full_log = NULL;
	
	PyErr_SetString(PyExc_Exception, wrapped_exeption);
	
	free(buf);
	if(full_log)
		free(wrapped_exeption);
	va_end(vl);
}


void __redirect_logs_to_exeption()
{
	pcilib_set_logger(pcilib_get_log_level(), 
					  pcilib_print_error_to_py,
					  pcilib_get_logger_context());
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

void add_pcilib_value_to_dict(pcilib_t* ctx, PyObject* dict, pcilib_value_t* val, const char *name)
{
    PyObject *py_val = (PyObject*)pcilib_get_value_as_pyobject(ctx, val, NULL);

	if(py_val)
        pcilib_pydict_set_item(dict,
                              PyString_FromString(name),
                              py_val);
	else
        pcilib_pydict_set_item(dict,
                              PyString_FromString("defvalue"),
                              PyString_FromString("invalid"));
}

PyObject * pcilib_convert_property_info_to_pyobject(pcilib_t* ctx, pcilib_property_info_t listItem)
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

PyObject * pcilib_convert_register_info_to_pyobject(pcilib_t* ctx, pcilib_register_info_t listItem)
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
    pcilib_set_value_from_register_value(ctx, &defval, listItem.defvalue);
    add_pcilib_value_to_dict(ctx, pylistItem, &defval, "defvalue");

    if(listItem.range)
    {
		pcilib_value_t minval = {0};
		pcilib_set_value_from_register_value(ctx, &minval, listItem.range->min);
	
		pcilib_value_t maxval = {0};
		pcilib_set_value_from_register_value(ctx, &maxval, listItem.range->max);
		
        PyObject* range = PyDict_New();
        add_pcilib_value_to_dict(ctx, range, &minval, "min");
        add_pcilib_value_to_dict(ctx, range, &maxval, "max");
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
			pcilib_set_value_from_register_value(ctx, &val, listItem.values[j].value);

			pcilib_value_t min = {0};
			pcilib_set_value_from_register_value(ctx, &min, listItem.values[j].min);
		
			pcilib_value_t max = {0};
			pcilib_set_value_from_register_value(ctx, &max, listItem.values[j].max);
            
            add_pcilib_value_to_dict(ctx, valuesItem, &val, "value");
            add_pcilib_value_to_dict(ctx, valuesItem, &min, "min");
            add_pcilib_value_to_dict(ctx, valuesItem, &max, "max");

            if(listItem.values[j].name)
                pcilib_pydict_set_item(valuesItem,
									  PyString_FromString("name"),
									  PyString_FromString(listItem.values[j].name));

            if(listItem.values[j].description)
                pcilib_pydict_set_item(valuesItem,
									  PyString_FromString("description"),
									  PyString_FromString(listItem.values[j].description));

            pcilib_pylist_append(values, valuesItem);
        }

        pcilib_pydict_set_item(pylistItem,
                              PyString_FromString("values"),
                              values);
    }

    return pylistItem;
}

Pcipywrap *new_Pcipywrap(const char* fpga_device, const char* model)
{
	//opening device
    pcilib_t* ctx = pcilib_open(fpga_device, model);
    if(!ctx)
    {
		set_python_exception("Failed pcilib_open(%s, %s)", fpga_device, model);
		return NULL;
	}
	Pcipywrap *self;
	self = (Pcipywrap *) malloc(sizeof(Pcipywrap));
	self->shared = 0;
	self->ctx = ctx;
	return self;
}

Pcipywrap *create_Pcipywrap(PyObject* ctx)
{
	if(!PyCObject_Check(ctx))
	{
        set_python_exception("Incorrect ctx type. Only PyCObject is allowed");
		return NULL;
	}
	
	Pcipywrap *self;
	self = (Pcipywrap *) malloc(sizeof(Pcipywrap));
	self->shared = 1;
	self->ctx = PyCObject_AsVoidPtr(ctx);
	return self;
}

void delete_Pcipywrap(Pcipywrap *self) {	
	if(!self->shared)
		pcilib_close(self->ctx);
		
	free(self);
}

PyObject* Pcipywrap_read_register(Pcipywrap *self, const char *regname, const char *bank)
{
	pcilib_value_t val = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
	err = pcilib_read_register(self->ctx, bank, regname, &reg_value);
	if(err)
	{
		set_python_exception("Failed pcilib_read_register");
		return NULL;
	}
	
	err = pcilib_set_value_from_register_value(self->ctx, &val, reg_value);
	if(err)
	{
		set_python_exception("Failed pcilib_set_value_from_register_value");
		return NULL;
	}

    return pcilib_get_value_as_pyobject(self->ctx, &val, NULL);
}

PyObject* Pcipywrap_write_register(Pcipywrap *self, PyObject* val, const char *regname, const char *bank)
{
	pcilib_value_t val_internal = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
    err = pcilib_set_value_from_pyobject(self->ctx, &val_internal, val);
	
	if(err)
	{
		set_python_exception("Failed pcilib_set_value_from_pyobject");
		return NULL;
	}
	
	
	reg_value = pcilib_get_value_as_register_value(self->ctx, &val_internal, &err);
	if(err)
	{
		set_python_exception("Failed pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
	err = pcilib_write_register(self->ctx, bank, regname, reg_value);
	
	if(err)
	{
		set_python_exception("Failed pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
    return PyInt_FromLong((long)1);
}

PyObject* Pcipywrap_get_property(Pcipywrap *self, const char *prop)
{	
	int err;
	pcilib_value_t val = {0};
	
	err  = pcilib_get_property(self->ctx, prop, &val);
	
	if(err)
	{
		set_python_exception("Failed pcilib_get_property, (error %i)", err);
		return NULL;
	}
	
    return pcilib_get_value_as_pyobject(self->ctx, &val, NULL);
}

PyObject* Pcipywrap_set_property(Pcipywrap *self, PyObject* val, const char *prop)
{
	int err;
	
	pcilib_value_t val_internal = {0};
    err = pcilib_set_value_from_pyobject(self->ctx, &val_internal, val);
	if(err)
	{
		set_python_exception("pcilib_set_value_from_pyobject, (error %i)", err);
		return NULL;
	}
	
	err  = pcilib_set_property(self->ctx, prop, &val_internal);
	if(err)
	{
		set_python_exception("pcilib_set_property, (error %i)", err);
		return NULL;
	}
	
	return PyInt_FromLong((long)1);
}

PyObject* Pcipywrap_get_registers_list(Pcipywrap *self, const char *bank)
{
	pcilib_register_info_t *list = pcilib_get_register_list(self->ctx, bank, PCILIB_LIST_FLAGS_DEFAULT);
	
	PyObject* pyList = PyList_New(0);
    for(int i = 0; i < ((pcilib_t*)self->ctx)->num_reg; i++)
    {
		//serialize item attributes
        PyObject* pylistItem = pcilib_convert_register_info_to_pyobject(self->ctx, list[i]);
        pcilib_pylist_append(pyList, pylistItem);
    }
    
    pcilib_free_register_info(self->ctx, list);
	
	return pyList;
}

PyObject* Pcipywrap_get_register_info(Pcipywrap *self, const char* reg,const char *bank)
{
    pcilib_register_info_t *info = pcilib_get_register_info(self->ctx, bank, reg, PCILIB_LIST_FLAGS_DEFAULT);

	if(!info)
	{
        return NULL;
	}

    PyObject* py_info = pcilib_convert_register_info_to_pyobject(self->ctx, info[0]);

    pcilib_free_register_info(self->ctx, info);

    return py_info;
}

PyObject* Pcipywrap_get_property_list(Pcipywrap *self, const char* branch)
{
    pcilib_property_info_t *list = pcilib_get_property_list(self->ctx, branch, PCILIB_LIST_FLAGS_DEFAULT);

    PyObject* pyList = PyList_New(0);

    for(int i = 0; list[i].path; i++)
    {
        //serialize item attributes
        PyObject* pylistItem = pcilib_convert_property_info_to_pyobject(self->ctx, list[i]);
        pcilib_pylist_append(pyList, pylistItem);
    }

    pcilib_free_property_info(self->ctx, list);

    return pyList;
}

PyObject* Pcipywrap_read_dma(Pcipywrap *self, unsigned char dma, size_t size)
{
	int err;
	void* buf = NULL;
	size_t real_size;
	
	err = pcilib_read_dma(self->ctx, dma, (uintptr_t)NULL, size, buf, &real_size);
	if(err)
	{
		set_python_exception("Failed pcilib_read_dma", err);
		return NULL;
	}
		
	
	PyObject* py_buf = PyByteArray_FromStringAndSize((const char*)buf, real_size);
	if(buf)
		free(buf);
		
	return py_buf;
}

PyObject* Pcipywrap_lock_global(Pcipywrap *self)
{
	int err;
	
	err = pcilib_lock_global(self->ctx);
	if(err)
	{
		set_python_exception("Failed pcilib_lock_global");
		return NULL;
	}

	return PyInt_FromLong((long)1);
}

void Pcipywrap_unlock_global(Pcipywrap *self)
{
	pcilib_unlock_global(self->ctx);
	return;
}

PyObject* Pcipywrap_lock(Pcipywrap *self, const char *lock_id) 
{   
   pcilib_lock_t* lock = pcilib_get_lock(self->ctx,
										  PCILIB_LOCK_FLAG_PERSISTENT,
										  lock_id);
    if(!lock)
	{
		set_python_exception("Failed pcilib_get_lock");
		return NULL;
	}
	
   
	int err = pcilib_lock(lock);
	if(err)
	{
		set_python_exception("Failed pcilib_lock");
		return NULL;
	}
	
	return PyInt_FromLong((long)1);
}

PyObject* Pcipywrap_try_lock(Pcipywrap *self, const char *lock_id)
{
	pcilib_lock_t* lock = pcilib_get_lock(self->ctx,
										  PCILIB_LOCK_FLAGS_DEFAULT,
										  lock_id);
    if(!lock)
	{
		set_python_exception("Failed pcilib_get_lock");
		return NULL;
	}
	
	int err = pcilib_try_lock(lock);
	if(err)
	{
		set_python_exception("Failed pcilib_try_lock");
		return NULL;
	}
	
	return PyInt_FromLong((long)1);
}

PyObject* Pcipywrap_unlock(Pcipywrap *self, const char *lock_id)
{
	pcilib_lock_t* lock = pcilib_get_lock(self->ctx,
										  PCILIB_LOCK_FLAGS_DEFAULT,
										  lock_id);
    if(!lock)
	{
		set_python_exception("Failed pcilib_get_lock");
		return NULL;
	}
	
	pcilib_unlock(lock);
	return PyInt_FromLong((long)1);
}


