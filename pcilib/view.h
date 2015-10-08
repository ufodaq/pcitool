#ifndef _PCILIB_VIEW_H
#define _PCILIB_VIEW_H

#include <pcilib.h>
#include <pcilib/unit.h>

#define PCILIB_VIEW_INVALID ((pcilib_view_t)-1)

//typedef void *pcilib_view_context_t;
typedef struct pcilib_view_context_s *pcilib_view_context_t;

typedef enum {
    PCILIB_VIEW_FLAG_PROPERTY = 1                                               /**< Indicates that view does not depend on a value and is independent property */
} pcilib_view_flags_t;

typedef struct {
    pcilib_version_t version;
    size_t description_size;
    pcilib_view_context_t (*init)(pcilib_t *ctx);
    void (*free)(pcilib_t *ctx, pcilib_view_context_t *view);
    int (*read_from_reg)(pcilib_t *ctx, pcilib_view_context_t *view, const pcilib_register_value_t *regval, pcilib_value_t *val);
    int (*write_to_reg)(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t *regval, const pcilib_value_t *val);
} pcilib_view_api_description_t;

typedef struct {
    const pcilib_view_api_description_t *api;
    pcilib_value_type_t type;			                                /**< The default data type returned by operation, PCILIB_VIEW_TYPE_STRING is supported by all operations */
    pcilib_view_flags_t flags;                                                  /**< Flags specifying type of the view */
    const char *unit;				                                /**< Returned unit (if any) */
    const char *name;				                                /**< Name of the view */
    const char *description;			                                /**< Short description */
} pcilib_view_description_t;

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_description_t *desc);

pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *view);
pcilib_view_t pcilib_find_register_view_by_name(pcilib_t *ctx, pcilib_register_t reg, const char *name);
pcilib_view_t pcilib_find_register_view(pcilib_t *ctx, pcilib_register_t reg, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* PCILIB_VIEW_H */
