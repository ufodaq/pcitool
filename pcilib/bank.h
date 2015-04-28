#ifndef _PCILIB_BANK_H
#define _PCILIB_BANK_H

#include <pcilib.h>

#define PCILIB_REGISTER_BANK_INVALID		((pcilib_register_bank_t)-1)
#define PCILIB_REGISTER_BANK0 			0					/**< First BANK to be used in the event engine */
#define PCILIB_REGISTER_BANK1 			1
#define PCILIB_REGISTER_BANK2 			2
#define PCILIB_REGISTER_BANK3 			3
#define PCILIB_REGISTER_BANK_DMA		64					/**< First BANK address to be used by DMA engines */
#define PCILIB_REGISTER_BANK_DYNAMIC		128					/**< First BANK address to map dynamic XML configuration */
#define PCILIB_REGISTER_PROTOCOL_INVALID	((pcilib_register_protocol_t)-1)
#define PCILIB_REGISTER_PROTOCOL0		0					/**< First PROTOCOL address to be used in the event engine */
#define PCILIB_REGISTER_PROTOCOL_DEFAULT	64					/**< Default memmap based protocol */
#define PCILIB_REGISTER_PROTOCOL_DMA		96					/**< First PROTOCOL address to be used by DMA engines */
#define PCILIB_REGISTER_PROTOCOL_DYNAMIC	128					/**< First PROTOCOL address to be used by plugins */

typedef uint8_t pcilib_register_bank_t;						/**< Type holding the bank position within the field listing register banks in the model */
typedef uint8_t pcilib_register_bank_addr_t;					/**< Type holding the bank address number */
typedef uint8_t pcilib_register_protocol_t;					/**< Type holding the protocol position within the field listing register protocols in the model */
typedef uint8_t pcilib_register_protocol_addr_t;				/**< Type holding the protocol address */


typedef struct pcilib_register_bank_context_s pcilib_register_bank_context_t;

typedef struct {
    pcilib_register_bank_context_t *(*init)(pcilib_t *ctx, pcilib_register_bank_t bank, const char *model, const void *args);			/**< Optional API call to initialize bank context */
    void (*free)(pcilib_register_bank_context_t *ctx);												/**< Optional API call to cleanup bank context */
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
    size_t size;								/**< Number of register addresses (plain addressing) in the bank (more register names can be defined if bit-fields/views are used) */
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
};


    // we don't copy strings, they should be statically allocated
int pcilib_init_register_banks(pcilib_t *ctx);
void pcilib_free_register_banks(pcilib_t *ctx);

int pcilib_add_register_banks(pcilib_t *ctx, size_t n, const pcilib_register_bank_description_t *banks);
int pcilib_add_register_protocols(pcilib_t *ctx, size_t n, const pcilib_register_protocol_description_t *protocols);
int pcilib_add_register_ranges(pcilib_t *ctx, size_t n, const pcilib_register_range_t *ranges);

pcilib_register_bank_t pcilib_find_register_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank);
pcilib_register_bank_t pcilib_find_register_bank_by_name(pcilib_t *ctx, const char *bankname);
pcilib_register_bank_t pcilib_find_register_bank(pcilib_t *ctx, const char *bank);

pcilib_register_protocol_t pcilib_find_register_protocol_by_addr(pcilib_t *ctx, pcilib_register_protocol_addr_t protocol);
pcilib_register_protocol_t pcilib_find_register_protocol_by_name(pcilib_t *ctx, const char *name);
pcilib_register_protocol_t pcilib_find_register_protocol(pcilib_t *ctx, const char *name);

#endif /* _PCILIB_BANK_H */
