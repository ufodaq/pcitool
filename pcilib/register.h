#ifndef _PCILIB_REGISTER_H
#define _PCILIB_REGISTER_H

#include <pcilib.h>
#include <pcilib/bank.h>
#include <pcilib/views.h>

#define PCILIB_REGISTER_NO_BITS			0
#define PCILIB_REGISTER_ALL_BITS		((pcilib_register_value_t)-1)

typedef enum {
    PCILIB_REGISTER_R = 1,			/**< reading from register is allowed */
    PCILIB_REGISTER_W = 2,			/**< normal writting to register is allowed */
    PCILIB_REGISTER_RW = 3,
    PCILIB_REGISTER_W1C = 4,			/**< writting 1 resets the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1C = 5,
    PCILIB_REGISTER_W1I = 8,			/**< writting 1 inversts the bit, writting 0 keeps the value */
    PCILIB_REGISTER_RW1I = 9,
} pcilib_register_mode_t;

typedef enum {
    PCILIB_REGISTER_STANDARD = 0,
    PCILIB_REGISTER_FIFO,
    PCILIB_REGISTER_BITS
} pcilib_register_type_t;

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
} pcilib_register_description_t;


typedef struct {
  pcilib_register_bank_t bank;		/**< Reference to bank containing the register */
  pcilib_register_value_t min, max;		/**< Minimum & maximum allowed values */
  pcilib_xml_node_t *xml;			/**< Additional XML properties */
  pcilib_view_t *views;      /** list of views linked to this register*/
} pcilib_register_context_t;


#ifdef __cplusplus
extern "C" {
#endif

int pcilib_add_registers(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_description_t *registers, pcilib_register_t *ids);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_REGISTER_H */
