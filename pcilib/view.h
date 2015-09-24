#ifndef _PCILIB_VIEW_H
#define _PCILIB_VIEW_H

#include <pcilib.h>
#include <pcilib/unit.h>

#define PCILIB_VIEW_INVALID ((pcilib_view_t)-1)

//typedef void *pcilib_view_context_t;
typedef struct pcilib_view_context_s *pcilib_view_context_t;

typedef struct {
    pcilib_version_t version;
    size_t description_size;
    pcilib_view_context_t (*init)(pcilib_t *ctx);
    void (*free)(pcilib_t *ctx, pcilib_view_context_t *view);
    int (*read_from_reg)(pcilib_t *ctx, pcilib_view_context_t *view, const pcilib_register_value_t *regval, pcilib_data_type_t viewval_type, size_t viewval_size, void *viewval);
    int (*write_to_reg)(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t *regval, pcilib_data_type_t viewval_type, size_t viewval_size, const void *viewval);
} pcilib_view_api_description_t;

typedef struct {
    const pcilib_view_api_description_t *api;
    pcilib_data_type_t type;			                                /**< The default data type returned by operation, PCILIB_VIEW_TYPE_STRING is supported by all operations */
    const char *unit;				                                /**< Returned unit (if any) */
    const char *name;				                                /**< Name of the view */
    const char *description;			                                /**< Short description */
} pcilib_view_description_t;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_description_t *desc);
pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *view);

#ifdef __cplusplus
}
#endif

#endif /* PCILIB_VIEW_H */
