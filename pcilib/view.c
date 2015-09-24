#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "pci.h"
#include "pcilib.h"
#include "view.h"
#include "error.h"


pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *view) {
    pcilib_view_t i;

    for(i = 0; ctx->views[i]; i++) {
        if (!strcasecmp(ctx->views[i]->name, view))
	    return i;
    }

    return PCILIB_VIEW_INVALID;
}



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
