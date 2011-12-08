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
#include <time.h>

#include "pci.h"

#include "tools.h"
#include "error.h"

#ifndef __timespec_defined
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#endif /* __timespec_defined */


pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event) {
    int i;
    pcilib_register_bank_t res;
    unsigned long addr;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_event_description_t *events = model_info->events;
    
    for (i = 0; events[i].name; i++) {
	if (!strcasecmp(events[i].name, event)) return events[i].evid;
    }

    return (pcilib_event_t)-1;
}

pcilib_event_data_type_t pcilib_find_event_data_type(pcilib_t *ctx, pcilib_event_t event, const char *data_type) {
    int i;
    pcilib_register_bank_t res;
    unsigned long addr;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    pcilib_event_data_type_description_t *data_types = model_info->data_types;
    
    for (i = 0; data_types[i].name; i++) {
	if ((data_types[i].evid&event)&&(!strcasecmp(data_types[i].name, data_type))) return data_types[i].data_type;
    }

    return (pcilib_event_data_type_t)-1;
}

int pcilib_init_event_engine(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;

//    api = pcilib_model[model].event_api;
    if ((api)&&(api->init)) {
	ctx->event_ctx = api->init(ctx);
	if (ctx->event_ctx) {
	    ctx->event_ctx->pcilib = ctx;
	}
    }
    
    return 0;
}

int pcilib_reset(pcilib_t *ctx) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }
    
    if (api->reset) 
	return api->reset(ctx->event_ctx);
	
    return 0;
}

int pcilib_configure_rawdata_callback(pcilib_t *ctx, pcilib_event_rawdata_callback_t callback, void *user) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    ctx->event_ctx->params.rawdata.callback = callback;
    ctx->event_ctx->params.rawdata.user = user;
    
    return 0;    
}

int pcilib_configure_autostop(pcilib_t *ctx, size_t max_events, pcilib_timeout_t duration) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    ctx->event_ctx->params.autostop.max_events = max_events;
    ctx->event_ctx->params.autostop.duration = duration;
    
    return 0;    
}

int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->start) 
	return api->start(ctx->event_ctx, event_mask, flags);

    return 0;
}

int pcilib_stop(pcilib_t *ctx, pcilib_event_flags_t flags) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->stop) 
	return api->stop(ctx->event_ctx, flags);

    return 0;
}

int pcilib_stream(pcilib_t *ctx, pcilib_event_callback_t callback, void *user) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->stream)
	return api->stream(ctx->event_ctx, callback, user);

    if (api->next_event) {
	pcilib_error("Streaming using next_event API is not implemented yet");
    }

    pcilib_error("Event enumeration is not suppored by API");
    return PCILIB_ERROR_NOTSUPPORTED;
}
/*
typedef struct {
    pcilib_event_id_t event_id;
    pcilib_event_info_t *info;
} pcilib_return_event_callback_context_t;

static int pcilib_return_event_callback(pcilib_event_id_t event_id, pcilib_event_info_t *info, void *user) {
    pcilib_return_event_callback_context_t *ctx = (pcilib_return_event_callback_context_t*)user;
    ctx->event_id = event_id;
    ctx->info = info;
}
*/

int pcilib_get_next_event(pcilib_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, pcilib_event_info_t **info) {
    int err;
    pcilib_event_api_description_t *api;
//    pcilib_return_event_callback_context_t user;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->next_event) 
	return api->next_event(ctx->event_ctx, timeout, evid, info);

/*	
    if (api->stream) {
	err = api->stream(ctx->event_ctx, 1, timeout, pcilib_return_event_callback, &user);
	if (err) return err;
	
	if (evid) *evid = user->event_id;
	if (info) *info = user->info;

	return 0;
    }
*/

    pcilib_error("Event enumeration is not suppored by API");
    return PCILIB_ERROR_NOTSUPPORTED;
}

int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    pcilib_event_api_description_t *api;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    api = model_info->event_api;
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
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    pcilib_event_api_description_t *api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, arg_size, arg, size, NULL);

    return NULL;
}

int pcilib_copy_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t size, void *buf, size_t *retsize) {
    void *res;
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    pcilib_event_api_description_t *api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->get_data) {
	res = api->get_data(ctx->event_ctx, event_id, data_type, arg_size, arg, &size, buf);
	if (!res) return PCILIB_ERROR_FAILED;
	
	if (retsize) *retsize = size;
	return 0;
    }	

    return PCILIB_ERROR_NOTSUPPORTED;
}


void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size) {
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    pcilib_event_api_description_t *api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return NULL;
    }

    if (api->get_data) 
	return api->get_data(ctx->event_ctx, event_id, data_type, 0, NULL, size, NULL);

    return NULL;
}

int pcilib_copy_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t size, void *buf, size_t *ret_size) {
    void *res;
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);

    pcilib_event_api_description_t *api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->get_data) {
	res = api->get_data(ctx->event_ctx, event_id, data_type, 0, NULL, &size, buf);
	if (!res) return PCILIB_ERROR_FAILED;
	
	if (ret_size) *ret_size = size;
	return 0;
    }

    return PCILIB_ERROR_NOTSUPPORTED;
}

int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id, void *data) {
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx);
    
    pcilib_event_api_description_t *api = model_info->event_api;
    if (!api) {
	pcilib_error("Event API is not supported by the selected model");
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    if (api->return_data) 
	return api->return_data(ctx->event_ctx, event_id, data);

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
    
    err = pcilib_return_data(user->ctx, event_id, data);
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

int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, pcilib_timeout_t timeout) {
    int err;
    struct timespec ts;
    pcilib_event_id_t eid;
    
    pcilib_grab_callback_user_data_t user = {ctx, size, data};
    
    err = pcilib_start(ctx, event_mask, PCILIB_EVENT_FLAGS_DEFAULT);
    if (!err) err = pcilib_trigger(ctx, event_mask, 0, NULL);
    if (!err) {
	err = pcilib_get_next_event(ctx, timeout, &eid, NULL);
	if (!err) pcilib_grab_callback(event_mask, eid, &user);
    }
    pcilib_stop(ctx, PCILIB_EVENT_FLAGS_DEFAULT);
    return err;
}
