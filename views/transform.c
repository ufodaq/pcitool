#define _PCILIB_VIEW_TRANSFORM_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "model.h"
#include "transform.h"
#include "py.h"
#include "error.h"
#include "pci.h"

static pcilib_view_context_t * pcilib_transform_view_init(pcilib_t *ctx, pcilib_view_t view) {
    int err;

    pcilib_view_context_t *view_ctx;
    pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(ctx->views[view]);

    if(v->script) {	
      pcilib_access_mode_t mode = 0;
         
      err = pcilib_py_load_script(ctx, v->script);
      if(err) {
          pcilib_error("Error (%i), loading script %s", err, v->script);
          return NULL;
      }
      
      err = pcilib_py_get_transform_script_properties(ctx, v->script, &mode);
           if(err) {
          pcilib_error("Error (%i) obtaining properties of transform script %s", err, v->script);
          return NULL;
      }

      if ((v->base.mode&PCILIB_REGISTER_RW) == 0)
         v->base.mode |= PCILIB_REGISTER_RW;
      v->base.mode &= (~PCILIB_REGISTER_RW)|mode;

      if (!v->read_from_reg) v->read_from_reg = "read_from_register";
      if (!v->write_to_reg) v->write_to_reg = "write_to_register";
    }
	
    view_ctx = (pcilib_view_context_t*)malloc(sizeof(pcilib_view_context_t));
    if (view_ctx) memset(view_ctx, 0, sizeof(pcilib_view_context_t));
    
    return view_ctx;
}

static int pcilib_transform_view_read(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t regval, pcilib_value_t *val) {
    int err;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(model_info->views[view_ctx->view]);

    err = pcilib_set_value_from_register_value(ctx, val, regval);
    if (err) return err;

    if (v->script)
	   err = pcilib_py_eval_func(ctx, v->script, v->read_from_reg, val);
    else 
	   err = pcilib_py_eval_string(ctx, v->read_from_reg, val);

    return err;
}

static int pcilib_transform_view_write(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t *regval, const pcilib_value_t *val) {
    int err = 0;
    pcilib_value_t val_copy = {0};

    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(model_info->views[view_ctx->view]);

    err = pcilib_copy_value(ctx, &val_copy, val);
    if (err) return err;

    if (v->script)
	   err = pcilib_py_eval_func(ctx, v->script, v->write_to_reg, &val_copy);
    else
	   err = pcilib_py_eval_string(ctx, v->write_to_reg, &val_copy);
		
    if (err) return err;
	
    *regval = pcilib_get_value_as_register_value(ctx, &val_copy, &err);
    return err;
}

const pcilib_view_api_description_t pcilib_transform_view_api =
  { PCILIB_VERSION, sizeof(pcilib_transform_view_description_t), pcilib_transform_view_init, NULL, NULL, pcilib_transform_view_read,  pcilib_transform_view_write };
