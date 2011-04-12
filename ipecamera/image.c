#define _IPECAMERA_IMAGE_C
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#include "../tools.h"
#include "../error.h"

#include "pcilib.h"

#include "model.h"
#include "image.h"

#define IPECAMERA_SLEEP_TIME 250000

struct ipecamera_s {
    pcilib_t *pcilib;

    char *data;
    size_t size;

    pcilib_callback_t cb;
    void *cb_user;
    
    int width;
    int height;

    pcilib_event_id_t event_id;

    pcilib_register_t control_reg, status_reg;
    pcilib_register_t start_reg, end_reg;
    pcilib_register_t lines_reg;
    pcilib_register_t exposure_reg;

    void *buffer;    
};


#define FIND_REG(var, bank, name)  \
        ctx->var = pcilib_find_register(pcilib, bank, name); \
	if (ctx->var ==  PCILIB_REGISTER_INVALID) { \
	    err = -1; \
	    pcilib_error("Unable to find a %s register", name); \
	}
    

#define GET_REG(reg, var) \
    err = pcilib_read_register_by_id(pcilib, ctx->reg, &var); \
    if (err) { \
	pcilib_error("Error reading %s register", ipecamera_registers[ctx->reg].name); \
	return err; \
    }

#define SET_REG(reg, val) \
    err = pcilib_write_register_by_id(pcilib, ctx->reg, val); \
    if (err) { \
	pcilib_error("Error writting %s register", ipecamera_registers[ctx->reg].name); \
	return err; \
    }

#define CHECK_REG(reg, check) \
    err = pcilib_read_register_by_id(pcilib, ctx->reg, &value); \
    if (err) { \
	pcilib_error("Error reading %s register", ipecamera_registers[ctx->reg].name); \
	return err; \
    } \
    if (!(check)) { \
	pcilib_error("Unexpected value (%li) of register %s", value, ipecamera_registers[ctx->reg].name); \
	return err; \
    }


void *ipecamera_init(pcilib_t *pcilib) {
    int err = 0; 
    
    ipecamera_t *ctx = malloc(sizeof(ipecamera_t));

    if (ctx) {
	ctx->pcilib = pcilib;

	ctx->data = pcilib_resolve_data_space(pcilib, 0, &ctx->size);
	if (!ctx->data) {
	    err = -1;
	    pcilib_error("Unable to resolve the data space");
	}
	
	ctx->buffer = malloc(1088 * 2048 * 2);
	if (!ctx->buffer) {
	    err = -1;
	    pcilib_error("Unable to allocate ring buffer");
	}
	
	FIND_REG(status_reg, "fpga", "status");
	FIND_REG(control_reg, "fpga", "control");
	FIND_REG(start_reg, "fpga", "start_address");
	FIND_REG(end_reg, "fpga", "end_address");

	FIND_REG(lines_reg, "cmosis", "number_lines");
	FIND_REG(exposure_reg, "cmosis", "exp_time");

	if (err) {
	    free(ctx);
	    return NULL;
	}
    }
    
    return (void*)ctx;
}

void ipecamera_free(void *vctx) {
    if (vctx) {
	ipecamera_t *ctx = (ipecamera_t*)vctx;
	
	if (ctx->buffer) free(ctx->buffer);
	free(ctx);
    }
}

int ipecamera_reset(void *vctx) {
    int err;
    pcilib_t *pcilib;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    pcilib_register_t control, status;
    pcilib_register_value_t value;
    
    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    pcilib = ctx->pcilib;
    control = ctx->control_reg;
    status = ctx->status_reg;

	// Set Reset bit to CMOSIS
    err = pcilib_write_register_by_id(pcilib, control, 5);
    if (err) {
	pcilib_error("Error setting CMOSIS reset bit");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);

	// Remove Reset bit to CMOSIS
    err = pcilib_write_register_by_id(pcilib, control, 1);
    if (err) {
	pcilib_error("Error reseting CMOSIS reset bit");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);

	// Special settings for CMOSIS v.2
    value = 01; err = pcilib_write_register_space(pcilib, "cmosis", 115, 1, &value);
    if (err) {
	pcilib_error("Error setting CMOSIS configuration");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);
    
    value = 07; err = pcilib_write_register_space(pcilib, "cmosis", 82, 1, &value);
    if (err) {
	pcilib_error("Error setting CMOSIS configuration");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);

	// This is temporary for verification purposes
    memset(ctx->data, 0, ctx->size);

    err = pcilib_read_register_by_id(pcilib, status, &value);
    if (err) {
	pcilib_error("Error reading status register");
	return err;
    }

    if (value != 0x0849FFFF) {
	pcilib_error("Unexpected value (%lx) of status register", value);
	return PCILIB_ERROR_VERIFY;
    }

    return 0;    
}

int ipecamera_start(void *vctx, pcilib_event_t event_mask, pcilib_callback_t cb, void *user) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    ctx->cb = cb;
    ctx->cb_user = user;
    
    
    ctx->event_id = 0;
    
    ctx->width = 1270;
    ctx->height = 1088; //GET_REG(lines_reg, lines);

    // allocate memory
    
    return 0;
}


int ipecamera_stop(void *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    return 0;
}


static int ipecamera_get_line(ipecamera_t *ctx, int line) {
    int err;
    pcilib_t *pcilib = ctx->pcilib;
    pcilib_register_value_t ptr, size, value;

    ipecamera_reset((void*)ctx);

    SET_REG(lines_reg, 1);
    
    SET_REG(control_reg, 149);
    usleep(IPECAMERA_SLEEP_TIME);
    CHECK_REG(status_reg, 0x0849FFFF);
    
    GET_REG(start_reg, ptr);
    GET_REG(end_reg, size);
    size -= ptr;

    printf("%i: %i %i\n", line, ptr, size);    
    
    SET_REG(control_reg, 141);
    usleep(IPECAMERA_SLEEP_TIME);
    CHECK_REG(status_reg, 0x0849FFFF);
}


static int ipecamera_get_image(ipecamera_t *ctx) {
    int err;
    int i;
    pcilib_t *pcilib = ctx->pcilib;

    for (i = 0; i < 1088; i++) {
	ipecamera_get_line(ctx, i);
    }
    
    ctx->event_id++;
}


int ipecamera_trigger(void *vctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    int err;
    pcilib_t *pcilib;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;

    }
    
    err = ipecamera_get_image(ctx);
    if (!err) err = ctx->cb(event, ctx->event_id, ctx->cb_user);

    return err;
}


void* ipecamera_get(void *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size) {
    if (size) *size = ctx->width * ctx->height * 2;
    return ctx->buffer;
}

int ipecamera_return(void *ctx, pcilib_event_id_t event_id) {
    return 0;
}
