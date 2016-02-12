#define _PCILIB_VIEW_TRANSFORM_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "model.h"
#include "transform.h"
#include "py.h"


static int pcilib_transform_view_read(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t regval, pcilib_value_t *val) {
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(model_info->views[view_ctx->view]);

	int err;

	err = pcilib_set_value_from_register_value(ctx, val, regval);
	if (err) return err;

	if(v->module)
		return pcilib_script_read(ctx, v->module, val);
	else
		return  pcilib_py_eval_string(ctx, v->read_from_reg, val);
}

static int pcilib_transform_view_write(pcilib_t *ctx, pcilib_view_context_t *view_ctx, pcilib_register_value_t *regval, const pcilib_value_t *val) {
	
    const pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(model_info->views[view_ctx->view]);

	int err = 0;
	
	pcilib_value_t val_copy = {0};
	err = pcilib_copy_value(ctx, &val_copy, val);
	if (err) return err;


	if(v->module)
		err = pcilib_script_write(ctx, v->module, &val_copy);
	else
		err = pcilib_py_eval_string(ctx, v->write_to_reg, &val_copy);
		
	if (err) return err;
	
	*regval = pcilib_get_value_as_register_value(ctx, &val_copy, &err);
	return err;
}

void pcilib_transform_view_free_description (pcilib_t *ctx, pcilib_view_description_t *view)
{
	pcilib_transform_view_description_t *v = (pcilib_transform_view_description_t*)(view);
	
	if(v->module)
		pcilib_py_free_script(v->module);
}


const pcilib_view_api_description_t pcilib_transform_view_api =
  { PCILIB_VERSION, sizeof(pcilib_transform_view_description_t), NULL, NULL, pcilib_transform_view_free_description, pcilib_transform_view_read,  pcilib_transform_view_write };
