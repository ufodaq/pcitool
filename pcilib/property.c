#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <views/register.h>

#include "pci.h"
#include "bank.h"
#include "view.h"
#include "register.h"
#include "property.h"

#include "tools.h"
#include "error.h"


int pcilib_add_registers_from_properties(pcilib_t *ctx, size_t n, pcilib_view_context_t* const *view_ctx, pcilib_view_description_t* const *v) {
    int err;
    
    pcilib_view_t i;
    pcilib_register_t pos = 0;
    pcilib_register_description_t regs[n];

    int access;
    pcilib_register_bank_t bank;

    bank = pcilib_find_register_bank_by_addr(ctx, pcilib_property_register_bank.addr);
    if (bank == PCILIB_REGISTER_BANK_INVALID) {
        err = pcilib_add_register_banks(ctx, 0, 1, &pcilib_property_register_bank, &bank);
        if (err) {
            pcilib_error("Error (%i) adding register bank (%s)", err, pcilib_property_register_bank.name);
            return err;
        }
    }

    access = ctx->banks[bank].access / 8;

    for (i = 0; i < n; i++) {
        if ((v[i]->flags&PCILIB_VIEW_FLAG_REGISTER) == 0) continue;

        regs[pos++] = (pcilib_register_description_t){
            .addr = view_ctx[i]->view * access,
            .bits = 8 * sizeof(pcilib_register_value_t),
            .mode = v[i]->mode,
            .type = PCILIB_REGISTER_PROPERTY,
            .bank = PCILIB_REGISTER_BANK_PROPERTY,
            .name = (v[i]->regname?v[i]->regname:v[i]->name),
            .description = v[i]->description
        };
    }

    if (pos)
        return pcilib_add_registers(ctx, 0, pos, regs, NULL);

    return 0;
}


int pcilib_add_properties_from_registers(pcilib_t *ctx, size_t n, const pcilib_register_bank_t *banks, const pcilib_register_description_t *registers) {
    int err;

    pcilib_register_t i;
    pcilib_view_t cur_view = ctx->num_views;

    if (!n) 
        return PCILIB_ERROR_INVALID_ARGUMENT;


    for (i = 0; i < n; i++) {
        char *view_name;
        pcilib_access_mode_t mode = 0;

        pcilib_register_view_description_t v;
        pcilib_register_bank_description_t *b = &ctx->banks[banks[i]];

        if (registers[i].type == PCILIB_REGISTER_PROPERTY) continue;

        view_name = malloc(strlen(registers[i].name) + strlen(b->name) + 13);
        if (!view_name) {
            pcilib_clean_views(ctx, cur_view);
            return PCILIB_ERROR_MEMORY;
        }

        sprintf(view_name, "/registers/%s/%s", b->name, registers[i].name);

        if ((registers[i].views)&&(registers[i].views[0].view)) {
            pcilib_view_t view = pcilib_find_view_by_name(ctx, registers[i].views[0].view);
            if (view == PCILIB_VIEW_INVALID) return PCILIB_ERROR_NOTFOUND;

            memcpy(&v, ctx->views[view], sizeof(pcilib_view_description_t));
            v.view = registers[i].views[0].name;

            if (ctx->views[view]->api->read_from_reg) mode |= PCILIB_ACCESS_R;
            if (ctx->views[view]->api->write_to_reg) mode |= PCILIB_ACCESS_W;
            mode &= ctx->views[view]->mode;
        } else {
            v.base.type = PCILIB_TYPE_LONG;
            mode = PCILIB_ACCESS_RW;
        }

        v.base.api = &pcilib_register_view_api;
        v.base.name = view_name;
        v.base.description = registers[i].description;
        v.base.mode = registers[i].mode&mode;
        v.base.flags |= PCILIB_VIEW_FLAG_PROPERTY;
        v.reg = registers[i].name;
        v.bank = b->name;

        err = pcilib_add_views(ctx, 1, (pcilib_view_description_t*)&v);
        if (err) {
            free(view_name);
            pcilib_clean_views(ctx, cur_view);
            return err;
        }
/*
        pcilib_view_context_t *view_ctx = pcilib_find_view_context_by_name(ctx, v.base.name);
        view_ctx->flags |= PCILIB_VIEW_FLAG_PROPERTY;
*/
    }

    return 0;
}

pcilib_property_info_t *pcilib_get_property_list(pcilib_t *ctx, const char *branch, pcilib_list_flags_t flags) {
    int err = 0;

    size_t pos = 0;
    size_t name_offset = 0;
    pcilib_view_context_t *view_ctx, *view_tmp;
    pcilib_property_info_t *info = (pcilib_property_info_t*)malloc((ctx->num_views + 1) * sizeof(pcilib_property_info_t));

    struct dir_hash_s {
        char *name;
        UT_hash_handle hh;
    } *dir_hash = NULL, *dir, *dir_tmp;

    if (branch) {
        name_offset = strlen(branch);
        if (branch[name_offset - 1] != '/') name_offset++;
    }

        // Find all folders
    HASH_ITER(hh, ctx->view_hash, view_ctx, view_tmp) {
        const pcilib_view_description_t *v = ctx->views[view_ctx->view];
        const char *subname = v->name + name_offset;
        const char *suffix;

        if (!(v->flags&PCILIB_VIEW_FLAG_PROPERTY)) continue;
        if ((branch)&&(strncasecmp(branch, v->name, strlen(branch)))) continue;

        suffix = strchr(subname, '/');
        if (suffix) {
            char *name = strndup(v->name, suffix - v->name);
            if (!name) {
                err = PCILIB_ERROR_MEMORY;
                break;
            }

            HASH_FIND_STR(dir_hash, name + name_offset, dir);
            if (dir) {
                free(name);
                continue;
            }


            dir = (struct dir_hash_s*)malloc(sizeof(struct dir_hash_s));
            if (!dir) {
                err = PCILIB_ERROR_MEMORY;
                break;
            }

            dir->name = name;

            HASH_ADD_KEYPTR(hh, dir_hash, dir->name + name_offset, strlen(dir->name + name_offset), dir);
        }
    }

    HASH_ITER(hh, ctx->view_hash, view_ctx, view_tmp) {
        const pcilib_view_description_t *v = ctx->views[view_ctx->view];
        const char *subname = v->name + name_offset;

        if (!(v->flags&PCILIB_VIEW_FLAG_PROPERTY)) continue;
        if ((branch)&&(strncasecmp(branch, v->name, strlen(branch)))) continue;

        if (!strchr(subname, '/')) {
            pcilib_view_context_t *found_view;

            char *path = strdup(v->name);
            if (!path) {
                err = PCILIB_ERROR_MEMORY;
                break;
            }
            char *name = strrchr(v->name, '/');
            if (name) name++;
            else name = path;

            HASH_FIND_STR(dir_hash, name, found_view);

            info[pos++] = (pcilib_property_info_t) {
                .name = name,
                .path = path,
                .description = v->description,
                .type = v->type,
                .mode = v->mode,
                .unit = v->unit,
                .flags = (found_view?PCILIB_LIST_FLAG_CHILDS:0)
            };

            if (found_view) HASH_DEL(dir_hash, found_view);
        }
    }

    HASH_ITER(hh, dir_hash, dir, dir_tmp) {
        char *name = strrchr(dir->name, '/');
        if (name) name++;
        else name = dir->name;

        info[pos++] = (pcilib_property_info_t) {
            .name = name,
            .path = dir->name,
            .type = PCILIB_TYPE_INVALID,
            .flags = PCILIB_LIST_FLAG_CHILDS
        };
    }

    HASH_CLEAR(hh, dir_hash);

    memset(&info[pos], 0, sizeof(pcilib_property_info_t));

    if (err) {
        pcilib_free_property_info(ctx, info);
        return NULL;
    }

    return info;
}

void pcilib_free_property_info(pcilib_t *ctx, pcilib_property_info_t *info) {
    int i;

    for (i = 0; info[i].path; i++)
        free((char*)info[i].path);
    free(info);
}

int pcilib_get_property(pcilib_t *ctx, const char *prop, pcilib_value_t *val) {
    return pcilib_read_register_view(ctx, NULL, NULL, prop, val);
}

int pcilib_set_property(pcilib_t *ctx, const char *prop, const pcilib_value_t *val) {
    return pcilib_write_register_view(ctx, NULL, NULL, prop, val);
}

int pcilib_get_property_attr(pcilib_t *ctx, const char *prop, const char *attr, pcilib_value_t *val) {
    pcilib_view_context_t *view_ctx;

    view_ctx = pcilib_find_view_context_by_name(ctx, prop);
    if (!view_ctx) {
        pcilib_error("The specified property (%s) is not found", prop);
        return PCILIB_ERROR_NOTFOUND;
    }

    if (!view_ctx->xml) return PCILIB_ERROR_NOTFOUND;

    return pcilib_get_xml_attr(ctx, view_ctx->xml, attr, val);
}
