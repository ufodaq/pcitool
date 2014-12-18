#define _KAPTURE_C
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

#include "../tools.h"
#include "../error.h"
#include "../event.h"

#include "pcilib.h"
#include "model.h"
#include "kapture.h"
#include "private.h"


pcilib_context_t *kapture_init(pcilib_t *vctx) {
    kapture_t *ctx = malloc(sizeof(kapture_t));

    if (ctx) {
	memset(ctx, 0, sizeof(kapture_t));
    }

    return ctx;
}

void kapture_free(pcilib_context_t *vctx) {
    if (vctx) {
	kapture_t *ctx = (kapture_t*)vctx;
	kapture_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	free(ctx);
    }
}

int kapture_reset(pcilib_context_t *ctx) {
}

int kapture_start(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags) {
}

int kapture_stop(pcilib_context_t *ctx, pcilib_event_flags_t flags) {
}

int kapture_trigger(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
}

int kapture_stream(pcilib_context_t *ctx, pcilib_event_callback_t callback, void *user) {
}

int kapture_next_event(pcilib_context_t *ctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info) {
}

int kapture_get(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void **data) {
}

int kapture_return(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data) {
}


