#ifndef _PCILIB_EVENT_H
#define _PCILIB_EVENT_H

#include "pcilib.h"

struct pcilib_event_api_description_s {
    pcilib_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_context_t *ctx);

    int (*reset)(pcilib_context_t *ctx);

    int (*start)(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags);
    int (*stop)(pcilib_context_t *ctx, pcilib_event_flags_t flags);
    int (*trigger)(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
    
    int (*stream)(pcilib_context_t *ctx, pcilib_event_callback_t callback, void *user);
    pcilib_event_id_t (*next_event)(pcilib_context_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, pcilib_event_info_t **info);

    void* (*get_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void *data);
    int (*return_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, void *data);
    
    pcilib_dma_context_t *(*init_dma)(pcilib_context_t *ctx);
};


typedef struct {
    size_t max_events;
    pcilib_timeout_t duration;
} pcilib_autostop_parameters_t;

typedef struct {
    pcilib_event_rawdata_callback_t callback;
    void *user;
} pcilib_rawdata_parameters_t;

typedef struct {
    pcilib_autostop_parameters_t autostop;
    pcilib_rawdata_parameters_t rawdata;
} pcilib_event_parameters_t;

struct pcilib_event_context_s {
    pcilib_event_parameters_t params;
    pcilib_t *pcilib;
};


int pcilib_init_event_engine(pcilib_t *ctx);

#endif /* _PCILIB_EVENT_H */
