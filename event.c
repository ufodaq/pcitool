#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "pci.h"

#include "tools.h"
#include "error.h"

pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event) {
    int i;
    pcilib_register_bank_t res;
    unsigned long addr;
    
    pcilib_model_t model = pcilib_get_model(ctx);
    pcilib_event_description_t *events = pcilib_model[model].events;
    
    for (i = 0; events[i].name; i++) {
	if (!strcasecmp(events[i].name, event)) return (1<<i);
    }

    return (pcilib_event_t)-1;
}


int pcilib_reset(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    if (api->reset) 
	return api->reset(ctx->event_ctx);
	
    return 0;
}

int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, void *callback, void *user) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->start) 
	return api->start(ctx->event_ctx, event_mask, callback, user);

    return 0;
}

int pcilib_stop(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->stop) 
	return api->stop(ctx->event_ctx);

    return 0;
}

pcilib_event_id_t pcilib_get_next_event(pcilib_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->next_event) 
	return api->next_event(ctx->event_ctx, event_mask, timeout);

    pcilib_error("Event enumeration is not suppored by API");
    return PCILIB_EVENT_ID_INVALID;
}

int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_t model = pcilib_get_model(ctx);

    api = pcilib_model[model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->trigger) 
	return api->trigger(ctx->event_ctx, event, trigger_size, trigger_data);

    pcilib_error("Self triggering is not supported by the selected model");
    return PCILIB_ERROR_NOTSUPPORTED;
}


void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, arg_size, arg, size);

    return NULL;
}

void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, 0, NULL, size);

    return NULL;
}

int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id) {
    pcilib_event_api_description_t *api = pcilib_model[ctx->model].event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->return_data) 
	return api->return_data(ctx->event_ctx, event_id);

    return 0;
}


typedef struct {
    pcilib_t *ctx;
    
    size_t *size;
    void **data;
} pcilib_grab_callback_user_data_t;

static int pcilib_grab_callback(pcilib_event_t event, pcilib_event_id_t event_id, void *vuser) {
    int err;
    void *data;
    size_t size;
    int allocated = 0;

    pcilib_grab_callback_user_data_t *user = (pcilib_grab_callback_user_data_t*)vuser;

    data = pcilib_get_data(user->ctx, event_id, PCILIB_EVENT_DATA, &size);
    if (!data) {
	pcilib_error("Error getting event data");
	return PCILIB_ERROR_FAILED;
    }
    
    if (*(user->data)) {
	if ((user->size)&&(*(user->size) < size)) {
	    pcilib_error("The supplied buffer does not have enough space to hold the event data. Buffer size is %z, but %z is required", user->size, size);
	    return PCILIB_ERROR_MEMORY;
	}

	*(user->size) = size;
    } else {
	*(user->data) = malloc(size);
	if (!*(user->data)) {
	    pcilib_error("Memory allocation (%i bytes) for event data is failed");
	    return PCILIB_ERROR_MEMORY;
	}
	if (*(user->size)) *(user->size) = size;
	allocated = 1;
    }
    
    memcpy(*(user->data), data, size);
    
    err = pcilib_return_data(user->ctx, event_id);
    if (err) {
	if (allocated) {
	    free(*(user->data));
	    *(user->data) = NULL;
	}
	pcilib_error("The event data had been overwritten before it was returned, data corruption may occur");
	return err;
    }
    
    return 0;
}

int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, const struct timespec *timeout) {
    int err;
    
    pcilib_grab_callback_user_data_t user = {ctx, size, data};
    
    err = pcilib_start(ctx, event_mask, pcilib_grab_callback, &user);
    if (!err) {
	if (timeout) nanosleep(timeout, NULL);
        else err = pcilib_trigger(ctx, event_mask, 0, NULL);
    }
    pcilib_stop(ctx);
    return 0;
}
