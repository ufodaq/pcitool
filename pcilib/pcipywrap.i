%module pcipywrap

extern PyObject* __createPcilibInstance(const char *fpga_device, const char *model = NULL);
extern PyObject* __setPcilib(PyObject* addr);

extern PyObject* read_register(const char *regname, const char *bank = NULL);
extern PyObject* write_register(PyObject* val, const char *regname, const char *bank = NULL);

extern PyObject* get_property(const char *prop);
extern PyObject* set_property(const char *prop, PyObject* val);
