%module pcipywrap


extern PyObject* createPcilibInstance(const char *fpga_device, const char *model = NULL);
extern PyObject* setPcilib(PyObject* addr);
extern void closeCurrentPcilibInstance();
extern PyObject* getCurrentPcilibInstance();

extern PyObject* read_register(const char *regname, const char *bank = NULL);
extern PyObject* write_register(PyObject* val, const char *regname, const char *bank = NULL);

extern PyObject* get_property(const char *prop);
extern PyObject* set_property(const char *prop, PyObject* val);

extern PyObject* get_register_list(const char *bank = NULL);
extern PyObject* get_register_info(const char* reg,const char *bank = NULL);

extern PyObject* get_property_info(const char* branch = NULL);
