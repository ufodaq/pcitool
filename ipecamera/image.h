#ifndef _IPECAMERA_IMAGE_H
#define _IPECAMERA_IMAGE_H

#include <stdio.h>

#include "ipecamera.h"
#include "pcilib.h"

typedef struct ipecamera_s ipecamera_t;

void *ipecamera_init(pcilib_t *pcilib);
void ipecamera_free(void *ctx);

int ipecamera_reset(void *ctx);
int ipecamera_start(void *ctx, pcilib_event_t event_mask, pcilib_callback_t cb, void *user);
int ipecamera_stop(void *ctx);
int ipecamera_trigger(void *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);

void* ipecamera_get(void *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
int ipecamera_return(void *ctx, pcilib_event_id_t event_id);


#endif /* _IPECAMERA_IMAGE_H */
