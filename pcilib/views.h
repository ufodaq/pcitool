#ifndef _PCILIB_VIEWS_H
#define _PCILIB_VIEWS_H

#include "pcilib.h"
#include "unit.h"

typedef struct pcilib_view_enum_s pcilib_view_enum_t;

typedef struct pcilib_view_formula_s pcilib_view_formula_t;

typedef struct pcilib_view_enum2_s pcilib_view_enum2_t;

typedef struct pcilib_view_s pcilib_view_t;

typedef struct pcilib_formula_s pcilib_formula_t;

typedef int (*pcilib_view_operation_t)(pcilib_t *ctx, void *params, char* string, int read_or_write, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

/**
 * new type to define an enum view
 */
struct pcilib_view_enum_s {
  const char *name; /**<corresponding string to value*/
  pcilib_register_value_t value, min, max; 
};

struct pcilib_formula_s{
  char* read_formula;
  char* write_formula;
};

/**
 * complete type for an enum view : name will be changed after with the previous one
 */
struct pcilib_view_enum2_s {
  const char* name;
  pcilib_view_enum_t* enums_list;
  const char* description;
};

struct pcilib_view_s{
  const char* name;
  const char* description;
  pcilib_view_operation_t op;
  void* parameters;
  pcilib_unit_t base_unit;
};
/**
 * new type to define a formula view
 */
struct pcilib_view_formula_s {
  const char *name; /**<name of the view*/
  const char *read_formula; /**< formula to parse to read from a register*/
  const char *write_formula; /**<formula to parse to write to a register*/
	// **see it later**        const char *unit; (?)
  const char *description;
  pcilib_unit_t base_unit;
};

/**
 * function to read a register using a view
 */
int pcilib_read_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size, void *value);

/**
 * function to write to a register using a view
 */
int pcilib_write_view(pcilib_t *ctx, const char *bank, const char *regname, const char *unit, size_t value_size, void *value);

int pcilib_add_views_enum(pcilib_t* ctx,size_t n, const pcilib_view_enum2_t* views);

int pcilib_add_views_formula(pcilib_t* ctx, size_t n, const pcilib_view_formula_t* views);

int operation_enum(pcilib_t *ctx, void *params, char* name, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

int operation_formula(pcilib_t *ctx, void *params, char* unit, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_t* views);

char* pcilib_view_str_sub(const char* s, unsigned int start, unsigned int end);

#endif
