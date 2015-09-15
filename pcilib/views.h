#ifndef _PCILIB_VIEWS_H
#define _PCILIB_VIEWS_H

#include "pcilib.h"

#define PCILIB_MAX_TRANSFORMS_PER_UNIT 16

typedef struct pcilib_transform_unit_s pcilib_transform_unit_t;

typedef struct pcilib_unit_s pcilib_unit_t; 

typedef struct pcilib_enum_s pcilib_enum_t;

typedef struct pcilib_view_s pcilib_view_t;

typedef struct pcilib_formula_s pcilib_formula_t;

typedef int (*pcilib_view_operation_t)(pcilib_t *ctx, void *params, char* string, int read_or_write, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

/**
 * type to save a transformation unit in the pcitool program
 */
struct pcilib_transform_unit_s{
  char *name;
  char *transform_formula;
};

/**
 * type to save a unit in the pcitool programm
 */
struct pcilib_unit_s{
  char* name;
  pcilib_transform_unit_t transforms[PCILIB_MAX_TRANSFORMS_PER_UNIT];
};

/**
 * new type to define an enum view
 */
struct pcilib_enum_s {
  const char *name; /**<corresponding string to value*/
  pcilib_register_value_t value, min, max; 
};

struct pcilib_formula_s{
  char* read_formula;
  char* write_formula;
};

struct pcilib_view_s{
  const char* name;
  const char* description;
  pcilib_view_operation_t op;
  void* parameters;
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

int operation_enum(pcilib_t *ctx, void *params, char* name, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

int operation_formula(pcilib_t *ctx, void *params, char* unit, int view2reg, pcilib_register_value_t *regval, size_t viewval_size, void* viewval);

int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_t* views);


/**
 * function to populate the ctx with units
 */
int pcilib_add_units(pcilib_t* ctx, size_t n, const pcilib_unit_t* units);



#endif
