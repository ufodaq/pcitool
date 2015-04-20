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

#define PCILIB_REGISTER_NO_BITS			0
#define PCILIB_REGISTER_ALL_BITS		((pcilib_register_value_t)-1)

typedef uint8_t pcilib_register_bank_t;						/**< Type holding the bank position within the field listing register banks in the model */
typedef uint8_t pcilib_register_bank_addr_t;					/**< Type holding the bank address number */
typedef uint8_t pcilib_register_protocol_t;					/**< Type holding the protocol position within the field listing register protocols in the model */
typedef uint8_t pcilib_register_protocol_addr_t;				/**< Type holding the protocol address */


typedef struct pcilib_register_bank_context_s pcilib_register_bank_context_t;

typedef struct {
    pcilib_register_bank_context_t *(*init)(pcilib_t *ctx, pcilib_register_bank_t bank, const char *model, const void *args);
    void (*free)(pcilib_register_bank_context_t *ctx);
    int (*read)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value);
    int (*write)(pcilib_t *pcilib, pcilib_register_bank_context_t *ctx, pcilib_register_addr_t addr, pcilib_register_value_t value);
} pcilib_register_protocol_api_description_t;

typedef struct {
    pcilib_register_protocol_addr_t addr;
    const pcilib_register_protocol_api_description_t *api;
    const char *model;
    const void *args;
    const char *name;
    const char *description;
} pcilib_register_protocol_description_t;

typedef struct {
    pcilib_register_bank_addr_t addr;

    pcilib_bar_t bar;				// optional
    size_t size;
    
    pcilib_register_protocol_addr_t protocol;

    uintptr_t read_addr;			// or offset if bar specified
    uintptr_t write_addr;			// or offset if bar specified
    pcilib_endianess_t raw_endianess;

    uint8_t access;
    pcilib_endianess_t endianess;
    
    const char *format;
    const char *name;
    const char *description;
} pcilib_register_bank_description_t;

/**
  * Default mappings
  */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    pcilib_register_bank_addr_t bank;
    long addr_shift;
} pcilib_register_range_t;



struct pcilib_register_bank_context_s {
    const pcilib_register_bank_description_t *bank;
    const pcilib_register_protocol_api_description_t *api;
};


    // we don't copy strings, they should be statically allocated
int pcilib_init_register_banks(pcilib_t *ctx);
void pcilib_free_register_banks(pcilib_t *ctx);
int pcilib_add_register_banks(pcilib_t *ctx, size_t n, const pcilib_register_bank_description_t *banks);

pcilib_register_bank_t pcilib_find_register_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank);
pcilib_register_bank_t pcilib_find_register_bank_by_name(pcilib_t *ctx, const char *bankname);
pcilib_register_bank_t pcilib_find_register_bank(pcilib_t *ctx, const char *bank);

pcilib_register_protocol_t pcilib_find_register_protocol_by_addr(pcilib_t *ctx, pcilib_register_protocol_addr_t protocol);
pcilib_register_protocol_t pcilib_find_register_protocol_by_name(pcilib_t *ctx, const char *name);
pcilib_register_protocol_t pcilib_find_register_protocol(pcilib_t *ctx, const char *name);

#endif /* _PCILIB_BANK_H */
