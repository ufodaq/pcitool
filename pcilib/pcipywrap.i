%module pcipywrap
/*extern void* __ctx;*/
extern void __initCtx(void* ctx);
extern void __createCtxInstance(const char *fpga_device, const char *model);
extern int read_register(const char *bank, const char *regname, void *value);
extern int ReadRegister(const char *reg);
