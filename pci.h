#ifndef _PCITOOL_PCI_H
#define _PCITOOL_PCI_H

#define PCILIB_REGISTER_TIMEOUT 10000		/**< us */

#include "driver/pciDriver.h"

#include "pcilib.h"


const pci_board_info *pcilib_get_board_info(pcilib_t *ctx);

#ifdef _PCILIB_PCI_C
# include "ipecamera/model.h"
# include "default.h"

pcilib_model_description_t pcilib_model[3] = {
    { NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL },
    { ipecamera_registers, ipecamera_register_banks, ipecamera_register_ranges, &ipecamera_image_api }
};

pcilib_protocol_description_t pcilib_protocol[3] = {
    { pcilib_default_read, pcilib_default_write },
    { ipecamera_read, ipecamera_write },
    { NULL, NULL }
};
#else
extern void (*pcilib_error)(const char *msg, ...);
extern void (*pcilib_warning)(const char *msg, ...);

extern pcilib_protocol_description_t pcilib_protocol[];
#endif /* _PCILIB_PCI_C */

#endif /* _PCITOOL_PCI_H */
