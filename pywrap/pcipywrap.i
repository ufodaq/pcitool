%module pcipywrap

%{
#include "pcipywrap.h"
%}

extern void redirect_logs_to_exeption();

typedef struct {
	%extend {
		pcipywrap(const char* fpga_device = "/dev/fpga0", const char* model = NULL);
		pcipywrap(PyObject* ctx){return create_pcipywrap(ctx);}
		~pcipywrap();
	
		PyObject* read_register(const char *regname = NULL, const char *bank = NULL);
		PyObject* write_register(PyObject* val, const char *regname, const char *bank = NULL);
	
		PyObject* get_property(const char *prop);
		PyObject* set_property(PyObject* val, const char *prop);
	
		PyObject* get_registers_list(const char *bank = NULL);
		PyObject* get_register_info(const char* reg,const char *bank = NULL);
		PyObject* get_property_list(const char* branch = NULL);
		PyObject* read_dma(unsigned char dma, size_t size);
		
		PyObject* lock_global();
		void unlock_global();
		
		PyObject* lock(const char *lock_id);
		PyObject* lock_persistent(const char *lock_id);
		PyObject* try_lock(const char *lock_id);
		PyObject* try_lock_persistent(const char *lock_id);
		PyObject* unlock(const char *lock_id);
		PyObject* unlock_persistent(const char *lock_id);
	}
} pcipywrap;
