#ifndef _PCILIB_MODEL_H
#define _PCILIB_MODEL_H

#include <bank.h>
#include <register.h>
#include <dma.h>
#include <event.h>

typedef struct {
    uint8_t access;
    pcilib_endianess_t endianess;

    const pcilib_register_description_t *registers;
    const pcilib_register_bank_description_t *banks;
    const pcilib_register_protocol_description_t *protocols;
    const pcilib_register_range_t *ranges;

    const pcilib_event_description_t *events;
    const pcilib_event_data_type_description_t *data_types;

    const pcilib_dma_description_t *dma;
    const pcilib_event_api_description_t *api;

    const char *name;
    const char *description;
} pcilib_model_description_t;


const pcilib_model_description_t *pcilib_get_model_description(pcilib_t *ctx);

#endif /* _PCILIB_MODEL_H */
