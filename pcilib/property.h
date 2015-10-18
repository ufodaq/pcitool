#ifndef _PCILIB_PROPERTY_H
#define _PCILIB_PROPERTY_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * This is internal function used to add property view for all model registers. It is automatically
 * called from pcilib_add_registers and should not be called by the users. On error no new views are 
 * initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] n - number of views to initialize. 
 * @param[in] banks - array containing a bank id for each of the considered registers
 * @param[in] desc - register descriptions
 * @return - error or 0 on success
 */
int pcilib_add_register_properties(pcilib_t *ctx, size_t n, const pcilib_register_bank_t *banks, const pcilib_register_description_t *desc);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_PROPERTY_H */





// free'd by user. Do we need it?

