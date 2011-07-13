#ifndef _PCILIB_EVENT_H
#define _PCILIB_EVENT_H

#include "pcilib.h"

struct pcilib_event_api_description_s {
    pcilib_context_t *(*init)(pcilib_t *ctx);
    void (*free)(pcilib_context_t *ctx);

    int (*reset)(pcilib_context_t *ctx);

    int (*start)(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_callback_t callback, void *user);
    int (*stop)(pcilib_context_t *ctx);
    int (*trigger)(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
    
    pcilib_event_id_t (*next_event)(pcilib_context_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout);
    void* (*get_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
    int (*return_data)(pcilib_context_t *ctx, pcilib_event_id_t event_id);
    
    pcilib_dma_context_t *(*init_dma)(pcilib_context_t *ctx);

};


#endif /* _PCILIB_EVENT_H */
