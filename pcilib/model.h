#ifndef _PCILIB_MODEL_H
#define _PCILIB_MODEL_H

#include <pcilib/bank.h>
#include <pcilib/register.h>
#include <pcilib/dma.h>
#include <pcilib/event.h>
#include <pcilib/export.h>


typedef struct {
    const pcilib_version_t interface_version;

    const pcilib_event_api_description_t *api;
    const pcilib_dma_description_t *dma;

    const pcilib_register_description_t *registers;
    const pcilib_register_bank_description_t *banks;
    const pcilib_register_protocol_description_t *protocols;
    const pcilib_register_range_t *ranges;

    const pcilib_event_description_t *events;
    const pcilib_event_data_type_description_t *data_types;

    const char *name;
    const char *description;
} pcilib_model_description_t;


#ifdef __cplusplus
extern "C" {
#endif

const pcilib_model_description_t *pcilib_get_model_description(pcilib_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_MODEL_H */
