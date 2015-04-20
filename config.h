#ifndef _PCILIB_CONFIG_H
#define _PCILIB_CONFIG_H


#include "register.h"
#include "dma.h"

/*
typedef struct {
    const char *alias;
    pcilib_register_protocol_description_t *protocol;
} pcilib_register_protocol_alias_t;


typedef struct {
    const char *alias;
    pcilib_dma_description_t dma;
} pcilib_dma_alias_t;

extern pcilib_register_protocol_alias_t pcilib_protocols[];
extern pcilib_dma_alias_t pcilib_dma[];
*/

extern const pcilib_register_protocol_description_t pcilib_protocols[];
extern const pcilib_dma_description_t pcilib_dma[];


//extern const pcilib_register_protocol_description_t *pcilib_protocol_default;

#endif /* _PCILIB_CONFIG_H */
