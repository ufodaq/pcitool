#ifndef _PCITOOL_PCI_H
#define _PCITOOL_PCI_H

#define PCILIB_MAX_BANKS 6
#define PCILIB_REGISTER_TIMEOUT 10000		/**< us */

#include <stdint.h>

#include "driver/pciDriver.h"
#include "kernel.h"

#define pcilib_memcpy pcilib_memcpy32
#define pcilib_datacpy pcilib_datacpy32

typedef struct pcilib_s pcilib_t;

typedef uint8_t pcilib_bar_t;			/**< Type holding the PCI Bar number */
typedef uint8_t pcilib_register_t;		/**< Type holding the register ID within the Bank */
typedef uint8_t pcilib_register_addr_t;		/**< Type holding the register ID within the Bank */
typedef uint8_t pcilib_register_bank_t;		/**< Type holding the register bank number */
typedef uint8_t pcilib_register_bank_addr_t;	/**< Type holding the register bank number */
typedef uint8_t pcilib_register_size_t;		/**< Type holding the size in bits of the register */
typedef uint32_t pcilib_register_value_t;	/**< Type holding the register value */

typedef enum {
    PCILIB_LITTLE_ENDIAN = 0,
    PCILIB_BIG_ENDIAN
} pcilib_endianess_t;

typedef enum {
    PCILIB_MODEL_DETECT,
    PCILIB_MODEL_PCI,
    PCILIB_MODEL_IPECAMERA
} pcilib_model_t;

typedef enum {
    PCILIB_REGISTER_R = 1,
    PCILIB_REGISTER_W = 2,
    PCILIB_REGISTER_RW = 3
} pcilib_register_mode_t;


typedef enum {
    IPECAMERA_REGISTER_PROTOCOL
} pcilib_register_protocol_t;

#define PCILIB_BAR_DETECT 		((pcilib_bar_t)-1)
#define PCILIB_REGISTER_INVALID		((pcilib_register_t)-1)
#define PCILIB_ADDRESS_INVALID		((uintptr_t)-1)
#define PCILIB_REGISTER_BANK_INVALID	((pcilib_register_bank_t)-1)
#define PCILIB_REGISTER_BANK0 		0

typedef struct {
    pcilib_register_bank_addr_t addr;
    size_t size;
    
    pcilib_register_protocol_t protocol;

    uintptr_t read_addr;
    uintptr_t write_addr;
    uint8_t raw_endianess;

    uint8_t access;
    uint8_t endianess;
    
    const char *name;
    const char *description;
} pcilib_register_bank_description_t;

typedef struct {
    pcilib_register_addr_t addr;
    pcilib_register_size_t bits;
    pcilib_register_value_t defvalue;
    pcilib_register_mode_t mode;

    pcilib_register_bank_t bank;
    
    const char *name;
    const char *description;
} pcilib_register_description_t;

/**
  * Default mappings
  */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    pcilib_register_bank_t bank;
} pcilib_register_range_t;

typedef struct {
    int (*read)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value);
    int (*write)(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value);
} pcilib_protocol_description_t;

#include "ipecamera.h"

typedef struct {
    pcilib_register_description_t *registers;
    pcilib_register_bank_description_t *banks;
    pcilib_register_range_t *ranges;
} pcilib_model_description_t;

#ifdef _PCILIB_PCI_C
pcilib_model_description_t pcilib_model[3] = {
    { NULL, NULL, NULL },
    { NULL, NULL, NULL },
    { ipecamera_registers, ipecamera_register_banks, ipecamera_register_ranges }
};

pcilib_protocol_description_t pcilib_protocol[2] = {
    { ipecamera_read, ipecamera_write },
    { NULL, NULL }
};
#else
extern void (*pcilib_error)(const char *msg, ...);
extern void (*pcilib_warning)(const char *msg, ...);

extern pcilib_model_description_t pcilib_model[];
extern pcilib_protocol_description_t pcilib_protocol[];
#endif /* _PCILIB_PCI_C */

int pcilib_set_error_handler(void (*err)(const char *msg, ...));


pcilib_t *pcilib_open(const char *device, pcilib_model_t model);
void pcilib_close(pcilib_t *ctx);

const pci_board_info *pcilib_get_board_info(pcilib_t *ctx);
pcilib_model_t pcilib_get_model(pcilib_t *ctx);

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar);
void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data);
char  *pcilib_resolve_register_address(pcilib_t *ctx, uintptr_t addr);

pcilib_register_bank_t pcilib_find_bank(pcilib_t *ctx, const char *bank);
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg);

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);

int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value);
int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value);

int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value);
int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value);

#endif /* _PCITOOL_PCI_H */
