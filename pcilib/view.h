#ifndef _PCILIB_VIEW_H
#define _PCILIB_VIEW_H

#include <uthash.h>

#include <pcilib.h>
#include <pcilib/unit.h>

#define PCILIB_VIEW_INVALID ((pcilib_view_t)-1)

typedef struct pcilib_view_context_s pcilib_view_context_t;
typedef struct pcilib_view_description_s pcilib_view_description_t;

typedef enum {
    PCILIB_VIEW_FLAG_PROPERTY = 1,                                              /**< Indicates that view does not depend on a value and is independent property */
    PCILIB_VIEW_FLAG_REGISTER = 2                                               /**< Indicates that view does not depend on a value and should be mapped to the register space */
} pcilib_view_flags_t;

typedef struct {
    pcilib_version_t version;                                                                                                           /**< Version */
    size_t description_size;                                                                                                            /**< The actual size of the description */
    pcilib_view_context_t *(*init)(pcilib_t *ctx);                                                                                      /**< Optional function which should allocated context used by read/write functions */
    void (*free)(pcilib_t *ctx, pcilib_view_context_t *view);                                                                           /**< Optional function which should clean context */
    void (*free_description)(pcilib_t *ctx, pcilib_view_description_t *view);                                                           /**< Optional function which shoud clean required parts of the extended description if non-static memory was used to initialize it */
    int (*read_from_reg)(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t regval, pcilib_value_t *val);              /**< Function which computes view value based on the passed the register value (view-based properties should not use register value) */
    int (*write_to_reg)(pcilib_t *ctx, pcilib_view_context_t *view, pcilib_register_value_t *regval, const pcilib_value_t *val);        /**< Function which computes register value based on the passed value (view-based properties are not required to set the register value) */
} pcilib_view_api_description_t;

struct pcilib_view_description_s {
    const pcilib_view_api_description_t *api;
    pcilib_view_flags_t flags;                                                  /**< Flags specifying type of the view */
    pcilib_value_type_t type;			                                /**< The default data type returned by operation, PCILIB_VIEW_TYPE_STRING is supported by all operations */
    pcilib_access_mode_t mode;                                                  /**< Specifies if the view is read/write-only */
    const char *unit;				                                /**< Returned unit (if any) */
    const char *name;				                                /**< Name of the view */
    const char *regname;                                                        /**< Specifies the register name if the view should be mapped to register space */
    const char *description;			                                /**< Short description */
};

struct pcilib_view_context_s {
    const char *name;
    pcilib_view_t view;
    pcilib_xml_node_t *xml;
    UT_hash_handle hh;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Use this function to add new view definitions into the model. It is error to re-register
 * already registered view. The function will copy the context of unit description, but name, 
 * transform, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new views are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] n - number of views to initialize. It is OK to pass 0 if protocols variable is NULL terminated (last member of protocols array have all members set to 0)
 * @param[in] desc - view descriptions
 * @return - error or 0 on success
 */
int pcilib_add_views(pcilib_t *ctx, size_t n, const pcilib_view_description_t *desc);

/**
 * Use this function to add new view definitions into the model. It is error to re-register
 * already registered view. The function will copy the context of unit description, but name, 
 * transform, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new views are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] n - number of views to initialize. It is OK to pass 0 if protocols variable is NULL terminated (last member of protocols array have all members set to 0)
 * @param[in] desc - view descriptions
 * @param[out] refs - fills allocated view contexts. On error context is undefined.
 * @return - error or 0 on success
 */
int pcilib_add_views_custom(pcilib_t *ctx, size_t n, const pcilib_view_description_t *desc, pcilib_view_context_t **refs);

/**
 * Destroys data associated with views. This is an internal function and will
 * be called during clean-up.
 * @param[in,out] ctx - pcilib context
 * @param[in] start - specifies first view to clean (used to clean only part of the views to keep the defined state if pcilib_add_views has failed)
 */
void pcilib_clean_views(pcilib_t *ctx, pcilib_view_t start);

pcilib_view_context_t *pcilib_find_view_context_by_name(pcilib_t *ctx, const char *view);
pcilib_view_context_t *pcilib_find_register_view_context_by_name(pcilib_t *ctx, pcilib_register_t reg, const char *name);
pcilib_view_context_t *pcilib_find_register_view_context(pcilib_t *ctx, pcilib_register_t reg, const char *name);

pcilib_view_t pcilib_find_view_by_name(pcilib_t *ctx, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* PCILIB_VIEW_H */
