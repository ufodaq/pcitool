#define _PCILIB_VIEW_TRANSFORM_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "model.h"
#include "transform.h"


static int pcilib_transform_view_read(pcilib_t *ctx, pcilib_view_context_t *view, const pcilib_register_value_t *regval, pcilib_value_t *val) {
/*    int j=0;
    pcilib_register_value_t value=0;
    char* formula=NULL;

    if(view2reg==0) {
        if(!(strcasecmp(unit,((pcilib_view_t*)viewval)->base_unit.name))) {
            formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
            if(!(formula)) {
                pcilib_error("can't allocate memory for the formula");
                return PCILIB_ERROR_MEMORY;
            }
            strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
            pcilib_view_apply_formula(ctx,formula,regval);
            return 0;
        }

        for(j=0; ((pcilib_view_t*)viewval)->base_unit.transforms[j].name; j++) {
            if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.transforms[j].name,unit))) {
                // when we have found the correct view of type formula, we apply the formula, that get the good value for return
                formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->read_formula));
                if(!(formula)) {
                    pcilib_error("can't allocate memory for the formula");
                    return PCILIB_ERROR_MEMORY;
                }
                strncpy(formula,((pcilib_formula_t*)params)->read_formula,strlen(((pcilib_formula_t*)params)->read_formula));
                pcilib_view_apply_formula(ctx,formula, regval);
                //	  pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.transforms[j],unit,regval);
                return 0;
            }
        }*/
}

static int pcilib_transform_view_write(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t *regval, pcilib_value_t *val) {
/*        if(!(strcasecmp(unit, ((pcilib_view_t*)viewval)->base_unit.name))) {
            formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
            strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen(((pcilib_formula_t*)params)->write_formula));
            pcilib_view_apply_formula(ctx,formula,regval);
            return 0;
        }

        for(j=0; ((pcilib_view_t*)viewval)->base_unit.transforms[j].name; j++) {
            if(!(strcasecmp(((pcilib_view_t*)viewval)->base_unit.transforms[j].name,unit))) {
                // when we have found the correct view of type formula, we apply the formula, that get the good value for return
                formula=malloc(sizeof(char)*strlen(((pcilib_formula_t*)params)->write_formula));
                strncpy(formula,((pcilib_formula_t*)params)->write_formula,strlen((( pcilib_formula_t*)params)->write_formula));
                //pcilib_view_apply_unit(((pcilib_view_t*)viewval)->base_unit.transforms[j],unit,regval);
                pcilib_view_apply_formula(ctx,formula,regval);
                // we maybe need some error checking there , like temp_value >min and <max
                return 0;
            }
        }
    free(formula);
    return PCILIB_ERROR_INVALID_REQUEST;*/
}


const pcilib_view_api_description_t pcilib_transform_view_api =
  { PCILIB_VERSION, sizeof(pcilib_transform_view_description_t), NULL, NULL, NULL, pcilib_transform_view_read,  pcilib_transform_view_write };
