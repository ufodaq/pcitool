#ifndef PCIPYWRAP_H
#define PCIPYWRAP_H

#include "pci.h"
#include "error.h"
#include <Python.h>

typedef struct {
  void* ctx;
  int shared;
} Pcipywrap;

/*!
 * \brief Redirect pcilib standart log stream to exeption text.
 * Logger will accumulate errors untill get message, starts with "#E".
 * After that, logger will write last error, and all accumulated errors
 * to Python exeption text
 */
void __redirect_logs_to_exeption();

/*!
 * \brief Wraps for pcilib_open function.
 * \param[in] fpga_device path to the device file [/dev/fpga0]
 * \param[in] model specifies the model of hardware, autodetected if NULL is passed
 * \return Pointer to pcilib_t, created by pcilib_open; NULL with exeption text, if failed.
 */
PyObject* create_pcilib_instance(const char *fpga_device, const char *model);

Pcipywrap *new_Pcipywrap(const char* fpga_device, const char* model);
Pcipywrap *create_Pcipywrap(PyObject* ctx);
void delete_Pcipywrap(Pcipywrap *self);

/*!
 * \brief Reads register value. Wrap for pcilib_read_register function.
 * \param[in] regname the name of the register
 * \param[in] bank should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * \return register value, can be integer or float type; NULL with exeption text, if failed.
 */
PyObject* Pcipywrap_read_register(Pcipywrap *self, const char *regname, const char *bank);

/*!
 * \brief Writes value to register. Wrap for pcilib_write_register function.
 * \param[in] val Register value, that needs to be set. Can be int, float or string.
 * \param[in] regname the name of the register
 * \param[in] bank should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* Pcipywrap_write_register(Pcipywrap *self, PyObject* val, const char *regname, const char *bank);

/*!
 * \brief Reads propety value. Wrap for pcilib_get_property function.
 * \param[in] prop property name (full name including path)
 * \return property value, can be integer or float type; NULL with exeption text, if failed.
 */
PyObject* Pcipywrap_get_property(Pcipywrap *self, const char *prop);

/*!
 * \brief Writes value to property. Wrap for pcilib_set_property function.
 * \param[in] prop property name (full name including path)
 * \param[in] val Property value, that needs to be set. Can be int, float or string.
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* Pcipywrap_set_property(Pcipywrap *self, PyObject* val, const char *prop);
PyObject* Pcipywrap_get_registers_list(Pcipywrap *self, const char *bank);
PyObject* Pcipywrap_get_register_info(Pcipywrap *self, const char* reg,const char *bank);
PyObject* Pcipywrap_get_property_list(Pcipywrap *self, const char* branch);

PyObject* Pcipywrap_read_dma(Pcipywrap *self, unsigned char dma, size_t size);

PyObject* Pcipywrap_lock_global(Pcipywrap *self);
void Pcipywrap_unlock_global(Pcipywrap *self);

/*!
 * \brief Wrap for pcilib_lock
 * \param lock_id lock identificator
 * \warning This function should be called only under Python standart threading lock.
 * Otherwise it will stuck with more than 1 threads. See /xml/test_pywrap/test_prop_mt.py
 * for example.
 * \return 1, serialized to PyObject or NULL with exeption text, if failed.
 */
PyObject* Pcipywrap_lock(Pcipywrap *self, const char *lock_id);

PyObject* Pcipywrap_try_lock(Pcipywrap *self, const char *lock_id);
PyObject* Pcipywrap_unlock(Pcipywrap *self, const char *lock_id);

#endif /* PCIPYWRAP_H */
