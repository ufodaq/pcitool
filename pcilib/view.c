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
    int err = 0;
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
        pcilib_view_context_t *view_ctx;

        ctx->views[ctx->num_views + i] = (pcilib_view_description_t*)malloc(v->api->description_size);
        if (!ctx->views[ctx->num_views + i]) {
            err = PCILIB_ERROR_MEMORY;
            break;
        }

        if (v->api->init) 
            view_ctx = v->api->init(ctx);
        else {
            view_ctx = (pcilib_view_context_t*)malloc(sizeof(pcilib_view_context_t));
            memset(view_ctx, 0, sizeof(pcilib_view_context_t));
        }

        view_ctx->view = ctx->num_views + i;
        view_ctx->name = v->name;

        if (!view_ctx) {
            free(ctx->views[ctx->num_views + i]);
            err = PCILIB_ERROR_MEMORY;
            break;
        }

        HASH_ADD_KEYPTR(hh, ctx->view_hash, view_ctx->name, strlen(view_ctx->name), view_ctx);
        memcpy(ctx->views[ctx->num_views + i], v, v->api->description_size);
        ptr += v->api->description_size;
    }
    ctx->views[ctx->num_views + i] = NULL;
    ctx->num_views += i;

    return err;
}

void pcilib_clean_views(pcilib_t *ctx) {
    pcilib_view_t i;
    pcilib_view_context_t *view_ctx, *tmp;

    if (ctx->unit_hash) {
        HASH_ITER(hh, ctx->view_hash, view_ctx, tmp) {
            const pcilib_view_description_t *v = ctx->views[view_ctx->view];

            HASH_DEL(ctx->view_hash, view_ctx);
            if (v->api->free) v->api->free(ctx, view_ctx);
            else free(view_ctx);
        }
    }

    for (i = 0; ctx->views[i]; i++) {
	if (ctx->views[i]->api->free_description) {
	    ctx->views[i]->api->free_description(ctx, ctx->views[i]);
	} else {
	    free(ctx->views[i]);
	}
    }

    ctx->views[0] = NULL;
    ctx->num_views = 0;
}

pcilib_view_context_t *pcilib_find_view_context_by_name(pcilib_t *ctx, const char *name) {
    pcilib_view_context_t *view_ctx = NULL;

    HASH_FIND_STR(ctx->view_hash, name, view_ctx);
    return view_ctx;
}

pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *name) {
    pcilib_view_context_t *view_ctx = pcilib_find_view_context_by_name(ctx, name);
    if (view_ctx) return view_ctx->view;
    return PCILIB_VIEW_INVALID;
}



pcilib_view_context_t *pcilib_find_register_view_context_by_name(pcilib_t *ctx, pcilib_register_t reg, const char *name) {
    pcilib_view_t i;
    pcilib_register_context_t *regctx = &ctx->register_ctx[reg];

    if (!regctx->views) return NULL;

    for (i = 0; regctx->views[i].name; i++) {
	if (!strcasecmp(name, regctx->views[i].name)) {
	    return pcilib_find_view_context_by_name(ctx, regctx->views[i].view);
	}
    }

    return NULL;
}

    // We expect symmetric units. Therefore, we don't distringuish if we read or write
static int pcilib_detect_register_view_and_unit(pcilib_t *ctx, pcilib_register_t reg, const char *name, pcilib_view_context_t **ret_view, pcilib_unit_transform_t **ret_trans) {
    pcilib_view_t i;
    pcilib_view_context_t *view_ctx;
    pcilib_view_description_t *view_desc;
    pcilib_register_context_t *regctx = &ctx->register_ctx[reg];

    if (!regctx->views) return PCILIB_ERROR_NOTFOUND;

	// Check if view is just a name of listed view
    view_ctx = pcilib_find_register_view_context_by_name(ctx, reg, name);
    if (view_ctx) {
        if (ret_view) *ret_view = view_ctx;
        if (ret_trans) *ret_trans = NULL;
        return 0;
    }

	// Check if view is a unit
    for (i = 0; regctx->views[i].name; i++) {
	pcilib_unit_transform_t *trans;

	view_ctx = pcilib_find_view_context_by_name(ctx, regctx->views[i].view);
	if (!view_ctx) continue;

        view_desc = ctx->views[view_ctx->view];
        if (view_desc->unit) {
	    trans = pcilib_find_transform_by_unit_names(ctx, view_desc->unit, name);
	    if (trans) {
	        if (ret_trans) *ret_trans = trans;
	        if (ret_view) *ret_view = pcilib_find_view_context_by_name(ctx, view_desc->name);
	        return 0;
	    }
	}
    }

    return PCILIB_ERROR_NOTFOUND;
}

pcilib_view_context_t *pcilib_find_register_view_context(pcilib_t *ctx, pcilib_register_t reg, const char *name) {
    int err;
    pcilib_view_context_t *view;

    err = pcilib_detect_register_view_and_unit(ctx, reg, name, &view, NULL);
    if (err) return NULL;

    return view;
}

typedef struct {
    pcilib_register_t reg;
    pcilib_view_context_t *view;
    pcilib_unit_transform_t *trans;
} pcilib_view_configuration_t;

static int pcilib_detect_view_configuration(pcilib_t *ctx, const char *bank, const char *regname, const char *view_cname, int write_direction, pcilib_view_configuration_t *cfg) {
    int err = 0;
    pcilib_view_t view;
    pcilib_view_context_t *view_ctx;
    pcilib_unit_transform_t *trans = NULL;
    pcilib_register_t reg = PCILIB_REGISTER_INVALID;

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
	
	if (unit_name) view_ctx = pcilib_find_register_view_context_by_name(ctx, reg, view_name);
	else err = pcilib_detect_register_view_and_unit(ctx, reg, view_name, &view_ctx, &trans);

	if ((err)||(!view_ctx)) {
	    pcilib_error("Can't find the specified view %s for register %s", view_name, regname);
	    return PCILIB_ERROR_NOTFOUND;
	}
    } else {
        view_ctx = pcilib_find_view_context_by_name(ctx, view_name);
	if (!view_ctx) {
	    pcilib_error("Can't find the specified view %s", view_name);
	    return PCILIB_ERROR_NOTFOUND;
	}
    }
    view = view_ctx->view;

    if (unit_name) {
        if (write_direction) trans = pcilib_find_transform_by_unit_names(ctx, unit_name, ctx->views[view]->unit);
        else trans = pcilib_find_transform_by_unit_names(ctx, ctx->views[view]->unit, unit_name);

        if (!trans) {
            pcilib_error("Don't know how to get the requested unit %s for view %s", unit_name, ctx->views[view]->name);
            return PCILIB_ERROR_NOTFOUND;
        }
    }

        // No transform is required
    if (!trans->transform) trans = NULL;

    cfg->reg = reg;
    cfg->view = view_ctx;
    cfg->trans = trans;

    return 0;
}


int pcilib_read_register_view(pcilib_t *ctx, const char *bank, const char *regname, const char *view, pcilib_value_t *val) {
    int err;

    pcilib_view_description_t *v;
    pcilib_view_configuration_t cfg;
    pcilib_register_value_t regvalue = 0;

    err = pcilib_detect_view_configuration(ctx, bank, regname, view, 0, &cfg);
    if (err) return err;

    v = ctx->views[cfg.view->view];

    if (!v->api->read_from_reg) {
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

    err = v->api->read_from_reg(ctx, cfg.view, regvalue, val);
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
    pcilib_value_t val = {0};

    pcilib_view_description_t *v;
    pcilib_view_configuration_t cfg;
    pcilib_register_value_t regvalue = 0;

    err = pcilib_detect_view_configuration(ctx, bank, regname, view, 1, &cfg);
    if (err) return err;

    v = ctx->views[cfg.view->view];

    if (!v->api->write_to_reg) {
        pcilib_error("The view (%s) does not support reading from the register", view);
        return PCILIB_ERROR_NOTSUPPORTED;
    }

    err = pcilib_copy_value(ctx, &val, valarg);
    if (err) return err;

    err = pcilib_convert_value_type(ctx, &val, v->type);
    if (err) {
        pcilib_error("Error (%i) converting the value of type (%s) to type (%s) used by view (%s)", pcilib_get_type_name(val.type), pcilib_get_type_name(v->type), view);
        return err;
    }

    if (cfg.trans) {
        err = pcilib_transform_unit(ctx, cfg.trans, &val);
        if (err) return err;
    }


    err = v->api->write_to_reg(ctx, cfg.view, &regvalue, &val);
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
