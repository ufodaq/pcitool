%module pcipywrap

%init %{
	init_pcipywrap_module();
%}

extern PyObject* create_pcilib_instance(const char *fpga_device, const char *model = NULL);
extern PyObject* set_pcilib(PyObject* addr);
extern void close_curr_pcilib_instance();
extern PyObject* get_curr_pcilib_instance();

extern PyObject* read_register(const char *regname, const char *bank = NULL);
extern PyObject* write_register(PyObject* val, const char *regname, const char *bank = NULL);

extern PyObject* get_property(const char *prop);
extern PyObject* set_property(PyObject* val, const char *prop);

extern PyObject* get_registers_list(const char *bank = NULL);
extern PyObject* get_register_info(const char* reg,const char *bank = NULL);

extern PyObject* get_property_info(const char* branch = NULL);
