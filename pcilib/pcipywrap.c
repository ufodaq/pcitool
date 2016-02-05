#include "pcilib.h"
#include <Python.h>
#include "pci.h"
#include "error.h"

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
 * \brief Sets pcilib context to wraper.
 * \param[in] addr Pointer to pcilib_t, serialized to bytearray
 */
void __setPcilib(PyObject* addr)
{
	if(!PyByteArray_Check(addr))
	{
		PyErr_SetString(PyExc_Exception, "Incorrect addr type. Only bytearray is allowed");
		return NULL;
	}
	
	//deserializing adress
	char* pAddr = PyByteArray_AsString(addr);
	
	//hard copy context adress
	for(int i = 0; i < sizeof(pcilib_t*) + 10; i++)
		((char*)&__ctx)[i] = pAddr[i];

	free(pAddr);
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
		PyErr_SetString(PyExc_Exception, "pcilib_t handler not initialized");
		return NULL;
	}
    
    pcilib_get_dma_description(__ctx);

	pcilib_value_t val = {0};
    pcilib_register_value_t reg_value;
	
	int err; 
	
	err = pcilib_read_register(__ctx, bank, regname, &reg_value);
	if(err)
	{
		PyErr_SetString(PyExc_Exception, "Failed: read_register");
		return NULL;
	}
	
	err = pcilib_set_value_from_register_value(__ctx, &val, reg_value);
	
	if(err)
	{
		PyErr_SetString(PyExc_Exception, "Failed: pcilib_set_value_from_register_value");
		return NULL;
	}

	switch(val.type)
	{
		case PCILIB_TYPE_INVALID:
			PyErr_SetString(PyExc_Exception, "Invalid register output type (PCILIB_TYPE_INVALID)");
			return NULL;
			
		case PCILIB_TYPE_STRING:
			PyErr_SetString(PyExc_Exception, "Invalid register output type (PCILIB_TYPE_STRING)");
			return NULL;
		
		case PCILIB_TYPE_LONG:
		{
			long ret;
			ret = pcilib_get_value_as_int(__ctx, &val, &err);
			
			if(err)
			{
				PyErr_SetString(PyExc_Exception, "Failed: pcilib_get_value_as_int");
				return NULL;
			}
			return PyInt_FromLong((long) ret);
		}
		
		case PCILIB_TYPE_DOUBLE:
		{
			double ret;
			ret = pcilib_get_value_as_float(__ctx, &val, &err);
			
			if(err)
			{
				PyErr_SetString(PyExc_Exception, "Failed: pcilib_get_value_as_int");
				return NULL;
			}
			return PyFloat_FromDouble((double) ret);
		}
		
		default:
			PyErr_SetString(PyExc_Exception, "Invalid register output type (unknown)");
			return NULL;
	}
}
