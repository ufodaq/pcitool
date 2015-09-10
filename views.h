typedef struct pcilib_view_enum_s pcilib_view_enum_t;

typedef struct pcilib_view_formula_s pcilib_view_formula_t;


/**
 * function to read a register using a view
 */
int pcilib_read_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view/*, const char *unit*/, size_t value_size, void *value);

/**
 * function to write to a register using a view
 */
int pcilib_write_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view/*, const char *unit*/, size_t value_size, void *value);

