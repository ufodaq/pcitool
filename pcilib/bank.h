#ifndef _PCILIB_BANK_H
#define _PCILIB_BANK_H

#include <pcilib.h>

#define PCILIB_REGISTER_BANK_INVALID		((pcilib_register_bank_t)-1)
#define PCILIB_REGISTER_BANK0 			0					/**< First BANK to be used in the event engine */
#define PCILIB_REGISTER_BANK1 			1
#define PCILIB_REGISTER_BANK2 			2
#define PCILIB_REGISTER_BANK3 			3
#define PCILIB_REGISTER_BANK_CONF		64					/**< Configuration registers */
#define PCILIB_REGISTER_BANK_PROPERTY           65                                      /**< Registers abstracting properties and other computed registers */
#define PCILIB_REGISTER_BANK_DMACONF		96					/**< DMA configuration in the software registers */
#define PCILIB_REGISTER_BANK_DMA		97					/**< First BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DMA0		PCILIB_REGISTER_BANK_DMA		/**< First BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DMA1		(PCILIB_REGISTER_BANK_DMA + 1)		/**< Second BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DMA2		(PCILIB_REGISTER_BANK_DMA + 2)		/**< Third BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DMA3		(PCILIB_REGISTER_BANK_DMA + 3)		/**< Fourth BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DYNAMIC		128					/**< First BANK address to map dynamic XML configuration */
#define PCILIB_REGISTER_PROTOCOL_INVALID	((pcilib_register_protocol_t)-1)
#define PCILIB_REGISTER_PROTOCOL0		0					/**< First PROTOCOL address to be used in the event engine */
#define PCILIB_REGISTER_PROTOCOL_DEFAULT	64					/**< Default memmap based protocol */
#define PCILIB_REGISTER_PROTOCOL_SOFTWARE	65					/**< Software registers */
#define PCILIB_REGISTER_PROTOCOL_PROPERTY       66                                      /**< Protocol to access registers interfacing properties */
#define PCILIB_REGISTER_PROTOCOL_DMA		96					/**< First PROTOCOL address to be used by DMA engines */
#define PCILIB_REGISTER_PROTOCOL_DYNAMIC	128					/**< First PROTOCOL address to be used by plugins */

typedef uint8_t pcilib_register_bank_t;						/**< Type holding the bank position within the field listing register banks in the model */
typedef uint8_t pcilib_register_bank_addr_t;					/**< Type holding the bank address number */
typedef uint8_t pcilib_register_protocol_t;					/**< Type holding the protocol position within the field listing register protocols in the model */
typedef uint8_t pcilib_register_protocol_addr_t;				/**< Type holding the protocol address */

typedef struct pcilib_register_bank_context_s pcilib_register_bank_context_t;

typedef enum {
    PCILIB_MODEL_MODIFICATON_FLAGS_DEFAULT = 0,
    PCILIB_MODEL_MODIFICATION_FLAG_OVERRIDE = 1,				/**< Instructs to override the existing registers/banks/etc... */
    PCILIB_MODEL_MODIFICATION_FLAG_SKIP_EXISTING = 2				/**< If flag is set, pcilib will just skip existing registers/banks/etc instead of reporting a error */
} pcilib_model_modification_flags_t;

typedef enum {
    PCILIB_ADDRESS_RESOLUTION_FLAGS_DEFAULT = 0,
    PCILIB_ADDRESS_RESOLUTION_FLAG_BUS_ADDRESS = 1,				/**< Resolve bus address instead of VA */
    PCILIB_ADDRESS_RESOLUTION_FLAG_PHYS_ADDRESS = 2,				/**< Resolve hardware address instead of VA */
    PCILIB_ADDRESS_RESOLUTION_MASK_ADDRESS_TYPE = 3,
    PCILIB_ADDRESS_RESOLUTION_FLAG_READ_ONLY = 4,				/**< Request read-only memory, in case if read-write resolution is impossible */
    PCILIB_ADDRESS_RESOLUTION_FLAG_WRITE_ONLY = 8,				/**< Request write-only resolution, in case if read-write resolution is impossible */
    PCILIB_ADDRESS_RESOLUTION_MASK_ACCESS_MODE = 12
} pcilib_address_resolution_flags_t;

typedef struct {
    pcilib_version_t version;

    pcilib_register_bank_context_t *(*init)(pcilib_t *ctx, pcilib_register_bank_t bank, const char *model, const void *args);			/**< Optional API call to initialize bank context */
    void (*free)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx);									/**< Optional API call to cleanup bank context */
    uintptr_t (*resolve)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx, pcilib_address_resolution_flags_t flags, pcilib_register_addr_t addr); /**< Resolves register virtual address (if supported) */
    int (*read)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value);		/**< Read from register, mandatory for RO/RW registers */
    int (*write)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx, pcilib_register_addr_t addr, pcilib_register_value_t value);		/**< Write to register, mandatory for WO/RW registers */
} pcilib_register_protocol_api_description_t;

typedef struct {
    pcilib_register_protocol_addr_t addr;					/**< Protocol address used in model for addressing the described protocol */
    const pcilib_register_protocol_api_description_t *api;			/**< Defines all API functions for protocol */
    const char *model;								/**< If NULL, the actually used model is used instead */
    const void *args;								/**< Custom protocol-specific arguments. The actual structure may depend on the specified model */
    const char *name;								/**< Short protocol name */
    const char *description;							/**< A bit longer protocol description */
} pcilib_register_protocol_description_t;

typedef struct {
    pcilib_register_bank_addr_t addr;						/**< Bank address used in model for addressing the described register bank */

    pcilib_register_protocol_addr_t protocol;					/**< Defines a protocol to access registers */
    pcilib_bar_t bar;								/**< Specifies the PCI BAR through which an access to the registers is provided (autodetcted if PCILIB_BAR_DETECT is specified) */
    uintptr_t read_addr;							/**< protocol specific (normally offset in the BAR of the first address used to read registers) */
    uintptr_t write_addr;							/**< protocol specific (normally offset in the BAR of the first address used to write registers) */

    uint8_t access;								/**< Default register size in bits (or word-size in plain addressing mode) */
    size_t size;								/**< The size of the bank in bytes (i.e. number of registers in plain addressing mode multiplied by access; this does not limit number of register names since indefinite number of names can be provided if bit-fields/views are used) */
    pcilib_endianess_t raw_endianess;						/**< Specifies endianess in the plain-addressing mode, PCILIB_HOST_ENDIAN have to be specified if no conversion desired. 
										Conversion applied after protocol. This value does not get into the account in register-access mode */
    pcilib_endianess_t endianess;						/**< Specifies endianess in the register-access mode, this may differ from raw_endianess if multi-word registers are used. 
										This is fully independent from raw_endianess. No double conversion is either performed */
    
    const char *format;								/**< printf format for the registers, either %lu for decimal output or 0x%lx for hexdecimal */
    const char *name;								/**< short bank name */
    const char *description;							/**< longer bank description */
} pcilib_register_bank_description_t;

/**
  * Default mappings: defines virtual address to register mappings, i.e. how 0x9000 in the following command 
  * will be mapped to the actual register. By comparing with start and end-addresses, we find to which range 
  * 0x9000 belongs to and detect actual bank and offset in it.
  * Simple example: pci -r 0x9000
  * if we specify range { 0x9000, 0x9100, 10, -0x9000}, the example command we print the value of the first
  * register in the bank 10.
  */
typedef struct {
    uintptr_t start;								/**< The first virtual address of the register range */
    uintptr_t end;								/**< The last virtual address of the register range */
    pcilib_register_bank_addr_t bank;						/**< The bank mapped to the specified range */
    long addr_shift;								/**< Address shift, i.e. how much we should add/substract to the virtual address to get address in the register bank */
} pcilib_register_range_t;



struct pcilib_register_bank_context_s {
    const pcilib_register_bank_description_t *bank;				/**< Corresponding bank description */
    const pcilib_register_protocol_api_description_t *api;			/**< API functions */
    pcilib_xml_node_t *xml;							/**< Additional XML properties */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initalizes context of register banks. This is an internal function and will
 * be called automatically when new register banks are added. On error no new
 * banks are initalized 
 * @param[in,out] ctx - pcilib context
 * @return - error or 0 on success
 */
int pcilib_init_register_banks(pcilib_t *ctx);

/**
 * Destroys contexts of register banks. This is an internal function and will
 * be called during clean-up. 
 * @param[in,out] ctx - pcilib context
 * @param[in] start - specifies first bank to clean (used to clean only part of the banks to keep the defined state if pcilib_init_register_banks has failed)
 */
void pcilib_free_register_banks(pcilib_t *ctx, pcilib_register_bank_t start);


/**
 * Use this function to add new register banks into the model or override configuration
 * of the existing banks. The function will copy the context of banks structure, but name, 
 * description, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new banks are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] flags - instructs if existing banks should be reported as error (default), overriden or ignored
 * @param[in] n - number of banks to initialize. It is OK to pass 0 if banks variable is NULL terminated (last member of banks array have all members set to 0)
 * @param[in] banks - bank descriptions
 * @param[out] ids - if specified will contain the ids of the newly registered and overriden banks
 * @return - error or 0 on success
 */
int pcilib_add_register_banks(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_bank_description_t *banks, pcilib_register_bank_t *ids);

/**
 * Use this function to add new register protocols into the model. It is error to re-register
 * already registered protocols. The function will copy the context of banks structure, but name, 
 * description, and other strings in the structure are considered to have static duration 
 * and will not be copied. On error no new protocols are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] flags - not used
 * @param[in] n - number of protocols to initialize. It is OK to pass 0 if protocols variable is NULL terminated (last member of protocols array have all members set to 0)
 * @param[in] protocols - protocol descriptions
 * @param[out] ids - if specified will contain the ids of the newly registered protocols
 * @return - error or 0 on success
 */
int pcilib_add_register_protocols(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_protocol_description_t *protocols, pcilib_register_protocol_t *ids);

/**
 * Use this function to add new register ranges into the model. It is error register
 * overlapping registered ranges. On error no new ranges are initalized.
 * @param[in,out] ctx - pcilib context
 * @param[in] flags - not used
 * @param[in] n - number of protocols to initialize. It is OK to pass 0 if protocols variable is NULL terminated.
 * @param[in] ranges - range descriptions
 * @return - error or 0 on success
 */
int pcilib_add_register_ranges(pcilib_t *ctx, pcilib_model_modification_flags_t flags, size_t n, const pcilib_register_range_t *ranges);

/**
 * Find the register bank id (offset in \a banks array) corresponding to the specified bank address 
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- the address of register bank
 * @return 		- bank id or PCILIB_REGISTER_BANK_INVALID if bank is not found
 */
pcilib_register_bank_t pcilib_find_register_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank);

/**
 * Find the register bank id (offset in \a banks array) corresponding to the specified bank name
 * @param[in,out] ctx	- pcilib context
 * @param[in] bankname	- the name of register bank
 * @return 		- bank id or PCILIB_REGISTER_BANK_INVALID if bank is not found
 */
pcilib_register_bank_t pcilib_find_register_bank_by_name(pcilib_t *ctx, const char *bankname);

/**
 * Find the register bank id (offset in \a banks array) corresponding to the specified bank name or address
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- either the name or the address of the required register bank
 * @return 		- bank id or PCILIB_REGISTER_BANK_INVALID if bank is not found
 */
pcilib_register_bank_t pcilib_find_register_bank(pcilib_t *ctx, const char *bank);

/**
 * Find the register protocol id (offset in \a protocols array) corresponding to the specified protocol address 
 * @param[in,out] ctx	- pcilib context
 * @param[in] protocol	- the address of register protocol
 * @return 		- protocol id or PCILIB_REGISTER_PROTOCOL_INVALID if register protocol is not found
 */
pcilib_register_protocol_t pcilib_find_register_protocol_by_addr(pcilib_t *ctx, pcilib_register_protocol_addr_t protocol);

/**
 * Find the register protocol id (offset in \a protocols array) corresponding to the specified protocol name
 * @param[in,out] ctx	- pcilib context
 * @param[in] name	- the name of register protocol
 * @return 		- protocol id or PCILIB_REGISTER_PROTOCOL_INVALID if register protocol is not found
 */
pcilib_register_protocol_t pcilib_find_register_protocol_by_name(pcilib_t *ctx, const char *name);

/**
 * Find the register protocol id (offset in \a protocols array) corresponding to the specified protocol name or address
 * @param[in,out] ctx	- pcilib context
 * @param[in] name	- either the name or the address of the required register protocol
 * @return 		- protocol id or PCILIB_REGISTER_PROTOCOL_INVALID if register protocol is not found
 */
pcilib_register_protocol_t pcilib_find_register_protocol(pcilib_t *ctx, const char *name);

/**
 * Resolves the address of the specified register bank. The address of the register0 in the bank is returned.
 *
 * All address types (virtual address in process address space, physical address, or bus address) can be resolved.
 * The caller has also to specify the requested access mode (read, write, read/write) and the error will be returned
 * if the requested access type is not possible.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] flags	- specifies the type of required address (virtual, physical, or bus) and the required access (ro/wo/rw)
 * @param[in] bank	- the id of register bank
 * @return 		- the resolved address or PCILIB_ADDRESS_INVALID on error
 */
uintptr_t pcilib_resolve_bank_address_by_id(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, pcilib_register_bank_t bank);

/**
 * Resolves the address of the specified register bank. The address of the register0 in the bank is returned.
 *
 * All address types (virtual address in process address space, physical address, or bus address) can be resolved.
 * The caller has also to specify the requested access mode (read, write, read/write) and the error will be returned
 * if the requested access type is not possible.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] flags	- specifies the type of required address (virtual, physical, or bus) and the required access (ro/wo/rw)
 * @param[in] bank	- the name of register bank
 * @return 		- the resolved address or PCILIB_ADDRESS_INVALID on error
 */
uintptr_t pcilib_resolve_bank_address(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, const char *bank);

/**
 * Resolves the address of the specified register.
 *
 * All address types (virtual address in process address space, physical address, or bus address) can be resolved.
 * The caller has also to specify the requested access mode (read, write, read/write) and the error will be returned
 * if the requested access type is not possible.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] flags	- specifies the type of required address (virtual, physical, or bus) and the required access (ro/wo/rw)
 * @param[in] reg	- the id of register
 * @return 		- the resolved address or PCILIB_ADDRESS_INVALID on error
 */
uintptr_t pcilib_resolve_register_address_by_id(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, pcilib_register_t reg);

/**
 * Resolves the address of the specified register.
 *
 * All address types (virtual address in process address space, physical address, or bus address) can be resolved.
 * The caller has also to specify the requested access mode (read, write, read/write) and the error will be returned
 * if the requested access type is not possible.
 *
 * @param[in,out] ctx	- pcilib context
 * @param[in] flags	- specifies the type of required address (virtual, physical, or bus) and the required access (ro/wo/rw)
 * @param[in] bank	- should specify the bank name if register with the same name may occur in multiple banks, NULL otherwise
 * @param[in] regname	- the name of the register
 * @return 		- the resolved address or PCILIB_ADDRESS_INVALID on error
 */
uintptr_t pcilib_resolve_register_address(pcilib_t *ctx, pcilib_address_resolution_flags_t flags, const char *bank, const char *regname);

/**
 * Extracts additional information about the specified register bank. The additional information
 * is model-specific and are provided as extra XML attributes in XML-described register bank.
 * The available attributes are only restricted by used XSD schema.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bank	- register bank id
 * @param[in] attr	- requested attribute name
 * @param[in,out] val	- the value of attribute is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_register_bank_attr_by_id(pcilib_t *ctx, pcilib_register_bank_t bank, const char *attr, pcilib_value_t *val);

/**
 * Extracts additional information about the specified register bank. The additional information
 * is model-specific and are provided as extra XML attributes in XML-described register bank.
 * The available attributes are only restricted by used XSD schema.
 * @param[in,out] ctx	- pcilib context
 * @param[in] bankname	- the name of register bank
 * @param[in] attr	- requested attribute name
 * @param[in,out] val	- the value of attribute is returned here (see \ref public_api_value),
 *			pcilib_clean_value() will be executed if \a val contains data. Therefore it should be always initialized to 0 before first use
 * @return		- error code or 0 on success
 */ 
int pcilib_get_register_bank_attr(pcilib_t *ctx, const char *bankname, const char *attr, pcilib_value_t *val);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_BANK_H */
