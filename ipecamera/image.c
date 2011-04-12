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
#define IPECAMERA_MAX_LINES 1088
#define IPECAMERA_DEFAULT_BUFFER_SIZE 10

typedef uint32_t ipecamera_payload_t;

struct ipecamera_s {
    pcilib_t *pcilib;

    char *data;
    size_t size;

    pcilib_callback_t cb;
    void *cb_user;

    pcilib_event_id_t event_id;

    pcilib_register_t control_reg, status_reg;
    pcilib_register_t start_reg, end_reg;
    pcilib_register_t n_lines_reg, line_reg;
    pcilib_register_t exposure_reg;

    int buffer_size;
    int buf_ptr;
    
    int width, height;

    ipecamera_pixel_t *buffer;
    ipecamera_change_mask_t *cmask;

    ipecamera_image_dimensions_t dim;
};


#define FIND_REG(var, bank, name)  \
        ctx->var = pcilib_find_register(pcilib, bank, name); \
	if (ctx->var ==  PCILIB_REGISTER_INVALID) { \
	    err = PCILIB_ERROR_NOTFOUND; \
	    pcilib_error("Unable to find a %s register", name); \
	}
    

#define GET_REG(reg, var) \
    err = pcilib_read_register_by_id(pcilib, ctx->reg, &var); \
    if (err) { \
	pcilib_error("Error reading %s register", ipecamera_registers[ctx->reg].name); \
    }

#define SET_REG(reg, val) \
    err = pcilib_write_register_by_id(pcilib, ctx->reg, val); \
    if (err) { \
	pcilib_error("Error writting %s register", ipecamera_registers[ctx->reg].name); \
    } \

#define CHECK_REG(reg, check) \
    if (!err) { \
	err = pcilib_read_register_by_id(pcilib, ctx->reg, &value); \
	if (err) { \
	    pcilib_error("Error reading %s register", ipecamera_registers[ctx->reg].name); \
	} \
	if (!(check)) { \
	    pcilib_error("Unexpected value (%li) of register %s", value, ipecamera_registers[ctx->reg].name); \
	    err = PCILIB_ERROR_INVALID_DATA; \
	} \
    }

#define CHECK_VALUE(value, val) \
    if ((!err)&&(value != val)) { \
	pcilib_error("Unexpected value (%x) in data stream (%x is expected)", value, val); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }

#define CHECK_FLAG(flag, check, ...) \
    if ((!err)&&(!(check))) { \
	pcilib_error("Unexpected value (%x) of " flag,  __VA_ARGS__); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }


void *ipecamera_init(pcilib_t *pcilib) {
    int err = 0; 
    
    ipecamera_t *ctx = malloc(sizeof(ipecamera_t));

    if (ctx) {
	memset(ctx, 0, sizeof(ipecamera_t));
	
	ctx->pcilib = pcilib;

	ctx->buffer_size = IPECAMERA_DEFAULT_BUFFER_SIZE;
	ctx->dim.bpp = sizeof(ipecamera_pixel_t) * 8;

	ctx->data = pcilib_resolve_data_space(pcilib, 0, &ctx->size);
	if (!ctx->data) {
	    err = -1;
	    pcilib_error("Unable to resolve the data space");
	}
	
	FIND_REG(status_reg, "fpga", "status");
	FIND_REG(control_reg, "fpga", "control");
	FIND_REG(start_reg, "fpga", "start_address");
	FIND_REG(end_reg, "fpga", "end_address");

	FIND_REG(n_lines_reg, "cmosis", "number_lines");
	FIND_REG(line_reg, "cmosis", "start1");
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
	ipecamera_stop(ctx);
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


	// Set default parameters
    SET_REG(n_lines_reg, 1);
    SET_REG(exposure_reg, 0);
    SET_REG(control_reg, 141);
    
    if (err) return err;
    
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
    int err = 0;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    ctx->cb = cb;
    ctx->cb_user = user;
    
    ctx->event_id = 0;
    ctx->buf_ptr = 0;    

    ctx->dim.width = 1270;
    ctx->dim.height = 1088; //GET_REG(lines_reg, lines);

    ctx->buffer = malloc(ctx->dim.width * ctx->dim.height * ctx->buffer_size * sizeof(ipecamera_pixel_t));
    if (!ctx->buffer) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate ring buffer");
    }

    ctx->cmask = malloc(ctx->dim.height * ctx->buffer_size * sizeof(ipecamera_change_mask_t));
    if (!ctx->cmask) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate change-mask buffer");
    }
    
    if (err) {
	ipecamera_stop(ctx);
	return err;
    }
    
    return 0;
}


int ipecamera_stop(void *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (ctx->buffer) {
	free(ctx->buffer);
	ctx->buffer = NULL;
    }
    
    if (ctx->cmask) {
	free(ctx->cmask);
	ctx->cmask = NULL;
    }


    ctx->event_id = 0;
    ctx->buf_ptr = 0;    
    
    return 0;
}


static int ipecamera_get_payload(ipecamera_t *ctx, ipecamera_pixel_t *pbuf, ipecamera_change_mask_t *cbuf, int line_req, pcilib_register_value_t size, ipecamera_payload_t *payload, pcilib_register_value_t *advance) {
    int i, j;
    int err = 0;
    
    ipecamera_payload_t info = payload[0];
    int channel = info&0x0F;		// 4 bits
    int line = (info>>4)&0x7FF;		// 11 bits
					// 1 bit is reserved
    int bpp = (info>>16)&0x0F;		// 4 bits
    int pixels = (info>>20)&0xFF;	// 8 bits
					// 2 bits are reserved
    int header = (info>>30)&0x03;	// 2 bits
    
    int bytes;
    
    CHECK_FLAG("payload header magick", header == 2, header);
    CHECK_FLAG("pixel size, only 10 bits are supported", bpp == 10, bpp);
    //CHECK_FLAG("row number, should be %li", line == line_req, line, line_req);
    CHECK_FLAG("relative row number, should be 0", line == 0, line);
    CHECK_FLAG("channel, limited by 10 channels now", channel < 10, channel);
    CHECK_FLAG("channel, dublicate entry for channel", ((*cbuf)&(1<<channel)) == 0, channel);
    CHECK_FLAG("number of pixels, 127 is expected", pixels == 127, pixels);
    
    bytes = pixels / 3;
    if (bytes * 3 < pixels) ++bytes;
    
    CHECK_FLAG("payload data bytes, at least %i are expected", bytes < size, size, bytes);
    
    for (i = 0; i < bytes; i++) {
	ipecamera_payload_t data = payload[i + 1];
        int header = (data>>30)&0x03;	
	
        CHECK_FLAG("payload data magick", header == 3, header);
	
	for (j = 0; j < 3; j++) {
	    int pix = 3 * i + j;

	    if (pix == pixels) break;
    
	    pbuf[channel*pixels + pix] = (data >> (10 * j))&0x3FF;
	}
    }
    
    if (!err) {
	*cbuf |= (1 << channel);
	*advance = bytes + 1;
    }

    return err;
}

static int ipecamera_get_line(ipecamera_t *ctx, ipecamera_pixel_t *pbuf, ipecamera_change_mask_t *cbuf, int line) {
    int err = 0;
    pcilib_t *pcilib = ctx->pcilib;
    pcilib_register_value_t ptr, size, pos, advance, value;
    ipecamera_payload_t *linebuf;
    int column = 0;

    ipecamera_reset((void*)ctx);

    SET_REG(n_lines_reg, 1);
    SET_REG(line_reg, line);
    
    SET_REG(control_reg, 149);
    usleep(IPECAMERA_SLEEP_TIME);
    CHECK_REG(status_reg, 0x0849FFFF);
    
    GET_REG(start_reg, ptr);
    GET_REG(end_reg, size);
    size -= ptr;

    pcilib_warning("Reading line %i: %i %i\n", line, ptr, size);    

    if (size < 6) {
	pcilib_error("The payload is tool small, we should have at least 5 header dwords and 1 footer.");
	return PCILIB_ERROR_INVALID_DATA;
    }
    
    linebuf = (uint32_t*)malloc(size * sizeof(ipecamera_payload_t));
    if (linebuf) {
	//pcilib_memcpy(linebuf, ctx->data + ptr, size * sizeof(ipecamera_payload_t));
	pcilib_datacpy(linebuf, ctx->data + ptr, sizeof(ipecamera_payload_t), size, pcilib_model[PCILIB_MODEL_IPECAMERA].endianess);

	
	CHECK_VALUE(linebuf[0], 0x51111111);	
	CHECK_VALUE(linebuf[1], 0x52222222);	
	CHECK_VALUE(linebuf[2], 0x53333333);	
	CHECK_VALUE(linebuf[3], 0x54444444);	
	CHECK_VALUE(linebuf[4], 0x55555555);
	
	if (err) {
	    size = 0;
	} else {
	    pos = 5;
	    size -= 6;
	}
	
	while (size > 0) {
	    err = ipecamera_get_payload(ctx, pbuf, cbuf, line, size, linebuf + pos, &advance);
	    if (err) break;
	    
	    pos += advance;
	    size -= advance;
	}
	
        CHECK_VALUE(linebuf[pos], 0x0AAAAAAA);

	CHECK_FLAG("payloads changed, we expect exactly 10 channels", *cbuf == 0x3FF, *cbuf);

	free(linebuf);
    }

    if (!err) {
	SET_REG(control_reg, 141);
	usleep(IPECAMERA_SLEEP_TIME);
	CHECK_REG(status_reg, 0x0849FFFF);
    }
    
    return err;
}


static int ipecamera_get_image(ipecamera_t *ctx) {
    int err;
    int i;
    int buf_ptr;
    pcilib_t *pcilib = ctx->pcilib;
    
//atomic    
    buf_ptr = ctx->buf_ptr;
    if (ctx->buf_ptr++ == ctx->buffer_size) ctx->buf_ptr = 0;
    if (ctx->event_id++ == 0) ctx->event_id = 1;

    memset(ctx->buffer + buf_ptr * ctx->dim.width * ctx->dim.height, 0, ctx->dim.width * ctx->dim.height * sizeof(ipecamera_pixel_t));
    memset(ctx->cmask + buf_ptr * ctx->dim.height, 0, ctx->dim.height * sizeof(ipecamera_change_mask_t));

    for (i = 0; i < ctx->dim.height; i++) {
	ipecamera_get_line(ctx, ctx->buffer + buf_ptr * ctx->dim.width * ctx->dim.height + i * ctx->dim.width,  ctx->cmask + buf_ptr * ctx->dim.height + i, i);
    }
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

static int ipecamera_resolve_event_id(ipecamera_t *ctx, pcilib_event_id_t evid) {
    int buf_ptr, diff;
    
    if ((!evid)||(evid > ctx->event_id)) return -1;
    
    diff = ctx->event_id - evid;
    buf_ptr = ctx->buf_ptr - diff - 1;
    if (buf_ptr < 0) {
	buf_ptr += ctx->buffer_size;
	if (buf_ptr < 0) return -1;
    }
    
    return buf_ptr;
}

void* ipecamera_get(void *vctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size) {
    int buf_ptr;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return NULL;
    }
    

    buf_ptr = ipecamera_resolve_event_id(ctx, event_id);

    //printf("%i %i %i\n", ctx->event_id, event_id, buf_ptr);

    if (buf_ptr < 0) return NULL;
    
    switch ((ipecamera_data_type_t)data_type) {
	case IPECAMERA_IMAGE_DATA:
	    if (size) *size = ctx->dim.width * ctx->dim.height * sizeof(ipecamera_pixel_t);
	    return ctx->buffer + buf_ptr * ctx->dim.width * ctx->dim.height;
	case IPECAMERA_CHANGE_MASK:
	    if (size) *size = ctx->dim.height * sizeof(ipecamera_change_mask_t);
	    return ctx->cmask + buf_ptr * ctx->dim.height;
	case IPECAMERA_DIMENSIONS:
	    if (size) *size = sizeof(ipecamera_image_dimensions_t);
	    return &ctx->dim;
	case IPECAMERA_IMAGE_REGION:
	case IPECAMERA_PACKED_IMAGE:
	    // Shall we return complete image or only changed parts?
	case IPECAMERA_PACKED_LINE:
	case IPECAMERA_PACKED_PAYLOAD:
	    pcilib_error("Support for data type (%li) is not implemented yet", data_type);
	    return NULL;
	default:
	    pcilib_error("Unknown data type (%li) is requested", data_type);
	    return NULL;
    }
}



int ipecamera_return(void *vctx, pcilib_event_id_t event_id) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;

    }

    if (ipecamera_resolve_event_id(ctx, event_id) < 0) {
	return PCILIB_ERROR_NOTAVAILABLE;
    }

    return 0;
}
