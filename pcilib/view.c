#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <alloca.h>

#include "pci.h"
#include "pcilib.h"
#include "view.h"
#include "error.h"
#include "value.h"

int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_description_t *desc) {
    size_t i;
    void *ptr;

    if (!n) {
            // No padding between array elements
        ptr = (void*)desc;
        while (1) {
            const pcilib_view_description_t *v = (const pcilib_view_description_t*)ptr;
            if (v->name) n++;
            else break;
            ptr += v->api->description_size;
        }
    }

    if ((ctx->num_views + n + 1) > ctx->alloc_views) {
        size_t size;
	pcilib_view_description_t **views;
        for (size = ctx->alloc_views; size < 2 * (n + ctx->num_views + 1); size<<=1);

        views = (pcilib_view_description_t**)realloc(ctx->views, size * sizeof(pcilib_view_description_t*));
        if (!views) return PCILIB_ERROR_MEMORY;

        ctx->views = views;
        ctx->alloc_views = size;

        ctx->model_info.views = (const pcilib_view_description_t**)views;
    }

        // No padding between array elements
    ptr = (void*)desc;
    for (i = 0; i < n; i++) {
        const pcilib_view_description_t *v = (const pcilib_view_description_t*)ptr;
        ctx->views[ctx->num_views + i] = (pcilib_view_description_t*)malloc(v->api->description_size);
        if (!ctx->views[ctx->num_views + i]) {
            size_t j;
            for (j = 0; j < i; j++)
                free(ctx->views[ctx->num_views + j]);
            ctx->views[ctx->num_views] = NULL;
            pcilib_error("Error allocating %zu bytes of memory for the view description", v->api->description_size);
            return PCILIB_ERROR_MEMORY;
        }
        memcpy(ctx->views[ctx->num_views + i], v, v->api->description_size);
        ptr += v->api->description_size;
    }
    ctx->views[ctx->num_views + i] = NULL;
    ctx->num_views += n;

    return 0;
}


pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *name) {
    pcilib_view_t i;

    for(i = 0; ctx->views[i]; i++) {
        if (!strcasecmp(ctx->views[i]->name, name))
	    return i;
    }

    return PCILIB_VIEW_INVALID;
}

pcilib_view_t pcilib_find_register_view_by_name(pcilib_t *ctx, pcilib_register_t reg, const char *name) {
    pcilib_view_t i;
    pcilib_register_context_t *regctx = &ctx->register_ctx[reg];

    if (!regctx->views) return PCILIB_VIEW_INVALID;

    for (i = 0; regctx->views[i].name; i++) {
	if (strcasecmp(name, regctx->views[i].name)) {
	    return pcilib_find_view_by_name(ctx, regctx->views[i].view);
	}
    }
    
    return PCILIB_VIEW_INVALID;
}

    // We expect symmetric units. Therefore, we don't distringuish if we read or write
pcilib_view_t pcilib_find_register_view(pcilib_t *ctx, pcilib_register_t reg, const char *name) {
    pcilib_view_t i;
    pcilib_register_context_t *regctx = &ctx->register_ctx[reg];

    if (!regctx->views) return PCILIB_VIEW_INVALID;

	// Check if view is just a name of listed view
    i = pcilib_find_register_view_by_name(ctx, reg, name);
    if (i != PCILIB_VIEW_INVALID) return i;

	// Check if view is a unit
    for (i = 0; regctx->views[i].name; i++) {
	pcilib_unit_transform_t *trans;
	pcilib_view_t view = pcilib_find_view_by_name(ctx, regctx->views[i].view);
	if (view == PCILIB_VIEW_INVALID) continue;
        
        if (ctx->views[view]->unit) {
	    trans = pcilib_find_transform_by_unit_names(ctx, ctx->views[view]->unit, name);
	    if (trans) return view;
	}
    }

    return PCILIB_VIEW_INVALID;
}


typedef struct {
    pcilib_register_t reg;
    pcilib_view_t view;
    pcilib_unit_transform_t *trans;
} pcilib_view_configuration_t;

static int pcilib_detect_view_configuration(pcilib_t *ctx, const char *bank, const char *regname, const char *view_cname, int write_direction, pcilib_view_configuration_t *cfg) {
    pcilib_view_t view;
    pcilib_register_t reg = PCILIB_REGISTER_INVALID;
    pcilib_unit_transform_t *trans = NULL;

    char *view_name = alloca(strlen(view_cname) + 1);
    char *unit_name;


    strcpy(view_name, view_cname);

    unit_name = strchr(view_name, ':');
    if (unit_name) {
	*unit_name = 0;
	unit_name++;
    }

    if (regname) {
	reg = pcilib_find_register(ctx, bank, regname);
	if (reg == PCILIB_REGISTER_INVALID) {
	    pcilib_error("Can't find the specified register %s", regname);
	    return PCILIB_ERROR_NOTFOUND;
	}
	
	// get value
	
	if (unit_name) view = pcilib_find_register_view_by_name(ctx, reg, view_name);
	else view = pcilib_find_register_view(ctx, reg, view_name);

	if (view == PCILIB_VIEW_INVALID) {
	    pcilib_error("Can't find the specified view %s for register %s", view_name, regname);
	    return PCILIB_ERROR_NOTFOUND;
	}
    } else {
        view = pcilib_find_view_by_name(ctx, view_name);
	if (view == PCILIB_VIEW_INVALID) {
	    pcilib_error("Can't find the specified view %s", view_name);
	    return PCILIB_ERROR_NOTFOUND;
	}
    }

    if (unit_name) {
        if (write_direction) trans = pcilib_find_transform_by_unit_names(ctx, unit_name, ctx->views[view]->unit);
        else trans = pcilib_find_transform_by_unit_names(ctx, ctx->views[view]->unit, unit_name);

        if (!trans) {
            pcilib_error("Don't know how to get the requested unit %s for view %s", unit_name, ctx->views[view]->name);
            return PCILIB_ERROR_NOTFOUND;
        }
            // No transform is required
        if (!trans->transform) trans = NULL;
    }

    cfg->reg = reg;
    cfg->view = view;
    cfg->trans = trans;

    return 0;
}


int pcilib_read_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, pcilib_value_t *val) {
    int err;

    pcilib_view_configuration_t cfg;
    pcilib_register_value_t regvalue = 0;

    err = pcilib_detect_view_configuration(ctx, bank, regname, view, 0, &cfg);
    if (err) return err;

    if (!ctx->views[cfg.view]->api->read_from_reg) {
        pcilib_error("The view (%s) does not support reading from the register", view);
        return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (regname) {
        err = pcilib_read_register_by_id(ctx, cfg.reg, &regvalue);
        if (err) {
            pcilib_error("Error (%i) reading register %s", err, regname);
            return err;
        }
    }

    pcilib_clean_value(ctx, val);

    err = ctx->views[cfg.view]->api->read_from_reg(ctx, NULL /*???*/, &regvalue, val);
    if (err) {
        if (regname) 
            pcilib_error("Error (%i) computing view (%s) of register %s", err, view, regname);
        else
            pcilib_error("Error (%i) computing view %s", err, view);
        return err;
    }

    if (cfg.trans) {
        err = pcilib_transform_unit(ctx, cfg.trans, val);
        if (err) return err;
    }

    return 0;
}


int pcilib_write_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, const pcilib_value_t *valarg) {
    int err;
    pcilib_value_t val;

    pcilib_view_configuration_t cfg;
    pcilib_register_value_t regvalue = 0;

    err = pcilib_detect_view_configuration(ctx, bank, regname, view, 1, &cfg);
    if (err) return err;

    if (!ctx->views[cfg.view]->api->write_to_reg) {
        pcilib_error("The view (%s) does not support reading from the register", view);
        return PCILIB_ERROR_NOTSUPPORTED;
    }

    err = pcilib_copy_value(ctx, &val, valarg);
    if (err) return err;

    err = pcilib_convert_value_type(ctx, &val, ctx->views[cfg.view]->type);
    if (err) {
        pcilib_error("Error (%i) converting the value of type (%s) to type (%s) used by view (%s)", pcilib_get_type_name(val.type), pcilib_get_type_name(ctx->views[cfg.view]->type), view);
        return err;
    }

    if (cfg.trans) {
        err = pcilib_transform_unit(ctx, cfg.trans, &val);
        if (err) return err;
    }


    err = ctx->views[cfg.view]->api->write_to_reg(ctx, NULL /*???*/, &regvalue, &val);
    if (err) {
        if (regname) 
            pcilib_error("Error (%i) computing view (%s) of register %s", err, view, regname);
        else
            pcilib_error("Error (%i) computing view %s", err, view);
        return err;
    }


    if (regname) {
        err = pcilib_write_register_by_id(ctx, cfg.reg, regvalue);
        if (err) {
            pcilib_error("Error (%i) reading register %s", err, regname);
            return err;
        }
    }

    return 0;
}
