#ifndef _PCITOOL_PCI_H
#define _PCITOOL_PCI_H

#define PCILIB_MAX_BANKS 6

#include <stdint.h>

#include "driver/pciDriver.h"
#include "kernel.h"

#define pcilib_memcpy memcpy32

typedef enum {
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

typedef struct {
    uint8_t id;
    uint8_t size;
    uint32_t defvalue;
    pcilib_register_mode_t mode;

    const char *name;
    
    pcilib_register_protocol_t protocol;
    uint64_t read_addr;
    uint64_t write_addr;

    const char *description;
} pcilib_register_t;

typedef struct {
    uint32_t start;
    uint32_t end;
} pcilib_register_range_t;

#include "ipecamera.h"

typedef struct {
    pcilib_register_t *registers;
    pcilib_register_range_t *ranges;
} pcilib_model_description_t;

#ifdef _PCILIB_PCI_C
pcilib_model_description_t pcilib_model_description[2] = {
    { NULL, NULL },
    { ipecamera_registers, ipecamera_register_range }
};
#else
extern pcilib_model_description_t pcilib_model_description[];
#endif /* _PCILIB_PCI_C */


int pcilib_open(const char *device);
void pcilib_close(int handle);

int pcilib_set_error_handler(void (*err)(const char *msg, ...));

const pci_board_info *pcilib_get_board_info(int handle);
pcilib_model_t pcilib_detect_model(int handle);
void *pcilib_map_bar(int handle, int bar);
void pcilib_unmap_bar(int handle, int bar, void *data);

int pcilib_read(void *buf, int handle, int bar, unsigned long addr, int size);
int pcilib_write(void *buf, int handle, int bar, unsigned long addr, int size);

int pcilib_find_register(pcilib_model_t model, const char *reg);

#endif /* _PCITOOL_PCI_H */
