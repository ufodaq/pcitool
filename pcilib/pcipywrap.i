%module pcipywrap

extern PyObject* read_register(const char *regname, const char *bank = NULL);
extern PyObject* __createPcilibInstance(const char *fpga_device, const char *model = NULL);
extern void __setPcilib(PyObject* addr);
