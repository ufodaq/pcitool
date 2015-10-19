#ifndef _PCILIB_REGISTER_H
#define _PCILIB_REGISTER_H

#include <uthash.h>

#include <pcilib.h>
#include <pcilib/bank.h>

#define PCILIB_REGISTER_NO_BITS			0
#define PCILIB_REGISTER_ALL_BITS		((pcilib_register_value_t)-1)


typedef enum {
    PCILIB_REGISTER_STANDARD = 0,               /**< Standard register */
    PCILIB_REGISTER_FIFO,                       /**< FIFO register */
    PCILIB_REGISTER_BITS,                       /**< Besides a big standard register, the register bit-fields may be described by bit registers */
    PCILIB_REGISTER_PROPERTY                    /**< A special register bound to a property and gettings/setting it value through it */
} pcilib_register_type_t;

typedef struct {
    const char *name;
    const char *view;
} pcilib_view_reference_t;

typedef struct {
    pcilib_register_addr_t addr;		/**< Register address in the bank */
    pcilib_register_size_t offset;		/**< Register offset in the byte (in bits) */
    pcilib_register_size_t bits;		/**< Register size in bits */
    pcilib_register_value_t defvalue;		/**< Default register value (some protocols, i.e. software registers, may set it during the initialization) */
    pcilib_register_value_t rwmask;		/**< Used to define how external bits of PCILIB_REGISTER_BITS registers are treated. 
						To keep bit value unchanged, we need to observe the following behavior depending on status of corresponding bit in this field:
						1 - standard bit (i.e. if we want to keep the bit value we need to read it, and, the write back), 
						0 - non-standard bit which behavior is defined by mode (only partially implemented. 
						    so far only 1C/1I modes (zero should be written to preserve the value) are supported */
    pcilib_register_mode_t mode;		/**< Register access (ro/wo/rw) and how writting to register works (if value just set as specified or, for instance, the bits which
						are on in the value are cleared/inverted). For information only, no preprocessing on bits is performed. */
    pcilib_register_type_t type;		/**< Defines type of register is it standard register, subregister for bit fields or view, fifo */
    pcilib_register_bank_addr_t bank;		/**< Specified the address of the bank this register belongs to */

    const char *name;				/**< The access name of the register */
    const char *description;			/**< Brief description of the register */

    pcilib_view_reference_t *views;		/**< List of supported views for this register */
} pcilib_register_description_t;

typedef struct {
    const char *name;                                                                   /**< Register name */
    pcilib_register_t reg;                                                              /**< Register index */
    pcilib_register_bank_t bank;							/**< Reference to bank containing the register */
    pcilib_register_value_range_t range;						/**< Minimum & maximum allowed values */
    pcilib_xml_node_t *xml;								/**< Additional XML properties */
    pcilib_view_reference_t *views;							/**< For non-static list of views, this vairables holds a copy of a NULL-terminated list from model (if present, memory should be de-allocated) */
    UT_hash_handle hh;
} pcilib_register_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Use this function to add new registers into the model. Currently, it is considered a error
 * to re-add already defined register. If it will turn out to be useful to redefine some registers 
 * from the model, it may change in the future. However, we should think how to treat bit-registers
 * in this case. The function will copy the context of registers structure, but name, 
 * description, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new registers are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] flags - not used now, but in future may instruct if existing registers should be reported as error (default), overriden or ignored
 * @param[in] n - number of registers to initialize. It is OK to pass 0 if registers array is NULL terminated (last member of the array have all members set to 0)
 * @param[in] registers - register descriptions
 * @param[out] ids - if specified will contain the ids of the newly registered and overriden registers
 * @return - error or 0 on success
 */
int pcilib_add_registers(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_description_t *registers, pcilib_register_t *ids);

/**
 * Destroys data associated with registers. This is an internal function and will
 * be called during clean-up.
 * @param[in,out] ctx - pcilib context
 * @param[in] start - specifies first register to clean (used to clean only part of the registers to keep the defined state if pcilib_add_registers has failed)
 */
void pcilib_clean_registers(pcilib_t *ctx, pcilib_register_t start);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_REGISTER_H */
