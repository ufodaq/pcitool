#ifndef _PCILIB_PROPERTY_H
#define _PCILIB_PROPERTY_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * This is an internal function used to add property view for all model registers. It is automatically
 * called from pcilib_add_registers and should not be called by the users. On error no new views are 
 * initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] n - number of views to initialize. 
 * @param[in] banks - array containing a bank id for each of the considered registers
 * @param[in] desc - register descriptions
 * @return - error or 0 on success
 */
int pcilib_add_properties_from_registers(pcilib_t *ctx, size_t n, const pcilib_register_bank_t *banks, const pcilib_register_description_t *registers);


/**
 * To reduce number of required interfaces, some of the property views may be also mapped into the 
 * model as registers. The client application, then, is able to use either register or property APIs
 * to access them. This is an internal function which processes the supplied views, finds which views
 * have to be mapped in the register space, and finally pushes corresponding registers into the model.
 * The function is automatically called from pcilib_add_views and should never be called by the user. 
 * On error no new registers are added.
 * @param[in,out] ctx - pcilib context
 * @param[in] n - number of views to analyze. 
 * @param[in] view_ctx - views to analyze
 * @param[in] view - array of pointers to corresponding view descriptions
 * @return - error or 0 on success
 */
int pcilib_add_registers_from_properties(pcilib_t *ctx, size_t n, pcilib_view_context_t* const *view_ctx, pcilib_view_description_t* const *view);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_PROPERTY_H */





// free'd by user. Do we need it?

