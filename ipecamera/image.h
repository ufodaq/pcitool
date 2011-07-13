#ifndef _IPECAMERA_IMAGE_H
#define _IPECAMERA_IMAGE_H

#include <stdio.h>

#include "ipecamera.h"
#include "pcilib.h"

pcilib_context_t *ipecamera_init(pcilib_t *pcilib);
void ipecamera_free(pcilib_context_t *ctx);

int ipecamera_reset(pcilib_context_t *ctx);
int ipecamera_start(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_callback_t cb, void *user);
int ipecamera_stop(pcilib_context_t *ctx);
int ipecamera_trigger(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
pcilib_event_id_t ipecamera_next_event(pcilib_context_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout);

void* ipecamera_get(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
int ipecamera_return(pcilib_context_t *ctx, pcilib_event_id_t event_id);

pcilib_dma_context_t *ipecamera_init_dma(pcilib_context_t *ctx);


#endif /* _IPECAMERA_IMAGE_H */
