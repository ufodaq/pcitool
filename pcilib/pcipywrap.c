#include "pci.h"
#include "error.h"
#include "pcipywrap.h"

/*!
 * \brief Global pointer to pcilib_t context.
 * Used by __setPcilib and read_register.
 */
pcilib_t* __ctx = 0;

/*!
 * \brief Wraps for pcilib_open function.
 * \param[in] fpga_device path to the device file [/dev/fpga0]
 * \param[in] model specifies the model of hardware, autodetected if NULL is passed
 * \return Pointer to pcilib_t, created by pcilib_open, serialized to bytearray
 */
PyObject* __createPcilibInstance(const char *fpga_device, const char *model)
{
	//opening device
    pcilib_t* ctx = pcilib_open(fpga_device, model);
    
    //serializing object
    return PyByteArray_FromStringAndSize((const char*)&ctx, sizeof(pcilib_t*));
}

/*!
 * \brief Sets python exeption text. Function interface same as printf.
 */
void setPyExeptionText(const char* msg, ...)
{
	char *buf;
	size_t sz;
	
	va_list vl, vl_copy;
    va_start(vl, msg);
	va_copy(vl_copy, vl);
	
	sz = vsnprintf(NULL, 0, msg, vl);
	buf = (char *)malloc(sz + 1);
	
	if(!buf)
	{
		PyErr_SetString(PyExc_Exception, "Memory error");
		return;
	}

	vsnprintf(buf, sz+1, msg, vl_copy);
	va_end(vl_copy);
	va_end(vl);
	
	PyErr_SetString(PyExc_Exception, buf);
	free(buf);
}

/*!
 * \brief Sets pcilib context to wraper.
 * \param[in] addr Pointer to pcilib_t, serialized to bytearray
 * \return true, serialized to PyObject, NULL with exeption text, if failed.
 */
PyObject* __setPcilib(PyObject* addr)
{
	if(!PyByteArray_Check(addr))
	{
		setPyExeptionText(PyExc_Exception, "Incorrect addr type. Only bytearray is allowed");
		return;
	}
	
	//deserializing adress
	char* pAddr = PyByteArray_AsString(addr);
	
	//hard copy context adress
	for(int i = 0; i < sizeof(pcilib_t*) + 10; i++)
		((char*)&__ctx)[i] = pAddr[i];

	free(pAddr);
	
	return PyInt_FromLong((long)1);
}

PyObject* pcilib_convert_val_to_pyobject(pcilib_t* ctx, pcilib_value_t *val, void (*errstream)(const char* msg, ...))
{
	int err;
	
	switch(val->type)
	{
		case PCILIB_TYPE_INVALID:
            errstream("Invalid register output type (PCILIB_TYPE_INVALID)");
			return NULL;
			
		case PCILIB_TYPE_STRING:
            errstream("Invalid register output type (PCILIB_TYPE_STRING)");
			return NULL;
		
		case PCILIB_TYPE_LONG:
		{
			long ret;
			ret = pcilib_get_value_as_int(__ctx, val, &err);
			
			if(err)
			{
                errstream("Failed: pcilib_get_value_as_int (%i)", err);
				return NULL;
			}
			return PyInt_FromLong((long) ret);
		}
		
		case PCILIB_TYPE_DOUBLE:
		{
			double ret;
			ret = pcilib_get_value_as_float(__ctx, val, &err);
			
			if(err)
			{
                errstream("Failed: pcilib_get_value_as_int (%i)", err);
				return NULL;
			}
			return PyFloat_FromDouble((double) ret);
		}
		
		default:
            errstream("Invalid register output type (unknown)");
			return NULL;
	}
}

int pcilib_convert_pyobject_to_val(pcilib_t* ctx, PyObject* pyVal, pcilib_value_t *val)
{
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


/*!
 * \brief Reads register value.
 * \param[in] regname the name of the register
 * \param[in] bank should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * \return register value, can be integer or float type
 */
PyObject* read_register(const char *regname, const char *bank)
{
	if(!__ctx)
	{
		setPyExeptionText("pcilib_t handler not initialized");
		return NULL;
	}
    
    pcilib_get_dma_description(__ctx);

	pcilib_value_t val = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
	err = pcilib_read_register(__ctx, bank, regname, &reg_value);
	if(err)
	{
		setPyExeptionText("Failed: read_register (%i)", err);
		return NULL;
	}
	
	err = pcilib_set_value_from_register_value(__ctx, &val, reg_value);
	if(err)
	{
		setPyExeptionText("Failed: pcilib_set_value_from_register_value (%i)", err);
		return NULL;
	}

    return pcilib_convert_val_to_pyobject(__ctx, &val, setPyExeptionText);
}


PyObject* get_property(const char *prop)
{
	if(!__ctx)
	{
		setPyExeptionText("pcilib_t handler not initialized");
		return NULL;
	}
	
	int err;
	pcilib_value_t val = {0};
	
	err  = pcilib_get_property(__ctx, prop, &val);
	
	if(err)
	{
			setPyExeptionText("Failed pcilib_get_property (%i)", err);
			return NULL;
	}
	
    return pcilib_convert_val_to_pyobject(__ctx, &val, setPyExeptionText);
}

PyObject* set_property(const char *prop, PyObject* val)
{
	int err;
	
	if(!__ctx)
	{
		setPyExeptionText("pcilib_t handler not initialized");
		return NULL;
	}
	
	pcilib_value_t val_internal = {0};
	err = pcilib_convert_pyobject_to_val(__ctx, val, &val_internal);
	if(err)
	{
		setPyExeptionText("pcilib_convert_pyobject_to_val (%i)", err);
		return NULL;
	}
	
	err  = pcilib_set_property(__ctx, prop, &val_internal);
	
	if(err)
	{
		setPyExeptionText("Failed pcilib_get_property (%i)", err);
		return NULL;
	}
	
	return PyInt_FromLong((long)1);
}

