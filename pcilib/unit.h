#ifndef _PCILIB_UNITS_H
#define _PCILIB_UNITS_H

#include "pcilib.h"

typedef struct pcilib_unit_s pcilib_unit_t; 
typedef struct pcilib_transform_unit_s pcilib_transform_unit_t;

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
  pcilib_transform_unit_t* other_units;
};

/**
 * function to populate the ctx with units
 */
int pcilib_add_units(pcilib_t* ctx, size_t n, const pcilib_unit_t* units);

#endif
