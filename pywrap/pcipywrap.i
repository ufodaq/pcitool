%module pcipywrap

%{
#include "pcipywrap.h"
%}

extern void __redirect_logs_to_exeption();

typedef struct {
	%extend {
		Pcipywrap(const char* fpga_device = NULL, const char* model = NULL);
		Pcipywrap(PyObject* ctx){return create_Pcipywrap(ctx);}
		~Pcipywrap();
	
		PyObject* read_register(const char *regname = NULL, const char *bank = NULL);
		PyObject* write_register(PyObject* val, const char *regname, const char *bank = NULL);
	
		PyObject* get_property(const char *prop);
		PyObject* set_property(PyObject* val, const char *prop);
	
		PyObject* get_registers_list(const char *bank = NULL);
		PyObject* get_register_info(const char* reg,const char *bank = NULL);
		PyObject* get_property_list(const char* branch = NULL);
	}
} Pcipywrap;
