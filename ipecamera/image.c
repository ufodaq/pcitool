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

#include "dma/nwl_dma.h"

#ifdef IPECAMERA_DEBUG
#include "dma/nwl.h"
#endif /* IPECAMERA_DEBUG */


#define IPECAMERA_SLEEP_TIME 250000
#define IPECAMERA_MAX_LINES 1088
#define IPECAMERA_DEFAULT_BUFFER_SIZE 10
#define IPECAMERA_EXPECTED_STATUS 0x08409FFFF
//#define IPECAMERA_EXPECTED_STATUS 0x0049FFFF

#define IPECAMERA_MAX_CHANNELS 16
#define IPECAMERA_PIXELS_PER_CHANNEL 128
#define IPECAMERA_WIDTH (IPECAMERA_MAX_CHANNELS * IPECAMERA_PIXELS_PER_CHANNEL)
#define IPECAMERA_HEIGHT 1088

//#define IPECAMERA_MEMORY 

//#define IPECAMERA_FRAME_REQUEST 0x149
//#define IPECAMERA_IDLE 		0x141
#define IPECAMERA_FRAME_REQUEST 0x1E9
#define IPECAMERA_READOUT	0x3E1
#define IPECAMERA_IDLE 		0x1E1

#define IPECAMERA_WRITE_RAW
#define IPECAMERA_REORDER_CHANNELS


#ifdef IPECAMERA_REORDER_CHANNELS
//int ipecamera_channel_order[10] = { 9, 7, 8, 6, 4, 2, 5, 1, 3, 0 };
int ipecamera_channel_order[IPECAMERA_MAX_CHANNELS] = { 15, 13, 14, 12, 10, 8, 11, 7, 9, 6, 5, 2, 4, 3, 0, 1 };
#endif 



typedef uint32_t ipecamera_payload_t;

struct ipecamera_s {
    pcilib_t *pcilib;

#ifndef IPECAMERA_DMA_ADDRESS    
    char *data;
#endif /* IPECAMERA_DMA_ADDRESS */
    size_t size;

    pcilib_event_callback_t cb;
    void *cb_user;

    pcilib_event_id_t event_id;
    pcilib_event_id_t reported_id;
    
#ifdef IPECAMERA_DMA_ADDRESS
    pcilib_dma_engine_t rdma, wdma;
#endif /* IPECAMERA_DMA_ADDRESS */

    pcilib_register_t packet_len_reg;
    pcilib_register_t control_reg, status_reg;
    pcilib_register_t start_reg, end_reg;
    pcilib_register_t n_lines_reg;
    uint16_t line_reg;
    pcilib_register_t exposure_reg;
    pcilib_register_t flip_reg;

    int started;
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
    if (!err) { \
	err = pcilib_read_register_by_id(pcilib, ctx->reg, &var); \
	if (err) { \
	    pcilib_error("Error reading %s register", ipecamera_registers[ctx->reg].name); \
	} \
    }

#define SET_REG(reg, val) \
    if (!err) { \
	err = pcilib_write_register_by_id(pcilib, ctx->reg, val); \
	if (err) { \
	    pcilib_error("Error writting %s register", ipecamera_registers[ctx->reg].name); \
	} \
    }

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
	pcilib_error("Unexpected value (0x%x) in data stream (0x%x is expected)", value, val); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }

#define CHECK_FLAG(flag, check, ...) \
    if ((!err)&&(!(check))) { \
	pcilib_error("Unexpected value (0x%x) of " flag,  __VA_ARGS__); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }


pcilib_context_t *ipecamera_init(pcilib_t *pcilib) {
    int err = 0; 
    
    ipecamera_t *ctx = malloc(sizeof(ipecamera_t));

    if (ctx) {
	memset(ctx, 0, sizeof(ipecamera_t));
	
	ctx->pcilib = pcilib;

	ctx->buffer_size = IPECAMERA_DEFAULT_BUFFER_SIZE;
	ctx->dim.bpp = sizeof(ipecamera_pixel_t) * 8;

#ifndef IPECAMERA_DMA_ADDRESS
	ctx->data = pcilib_resolve_data_space(pcilib, 0, &ctx->size);
	if (!ctx->data) {
	    err = -1;
	    pcilib_error("Unable to resolve the data space");
	}
#endif /* IPECAMERA_DMA_ADDRESS */
	
	FIND_REG(status_reg, "fpga", "status");
	FIND_REG(control_reg, "fpga", "control");
	FIND_REG(start_reg, "fpga", "start_address");
	FIND_REG(end_reg, "fpga", "end_address");

	FIND_REG(n_lines_reg, "cmosis", "number_lines");
	FIND_REG(line_reg, "cmosis", "start1");
	FIND_REG(exposure_reg, "cmosis", "exp_time");
	FIND_REG(flip_reg, "cmosis", "image_flipping");
	
	FIND_REG(packet_len_reg, "dma", "xrawdata_packet_length");

	ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
	ctx->wdma = PCILIB_DMA_ENGINE_INVALID;


	if (err) {
	    free(ctx);
	    return NULL;
	}
    }
    
    return (pcilib_context_t*)ctx;
}

void ipecamera_free(pcilib_context_t *vctx) {
    if (vctx) {
	ipecamera_t *ctx = (ipecamera_t*)vctx;
	ipecamera_stop(vctx);
	free(ctx);
    }
}

pcilib_dma_context_t *ipecamera_init_dma(pcilib_context_t *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(ctx->pcilib);
    if ((!model_info->dma_api)||(!model_info->dma_api->init)) {
	pcilib_error("The DMA engine is not configured in model");
	return NULL;
    }

#ifdef IPECAMERA_DMA_R3
    return model_info->dma_api->init(ctx->pcilib, PCILIB_NWL_MODIFICATION_IPECAMERA, NULL);
#else
    return model_info->dma_api->init(ctx->pcilib, PCILIB_DMA_MODIFICATION_DEFAULT, NULL);
#endif
}


int ipecamera_set_buffer_size(ipecamera_t *ctx, int size) {
    if (ctx->started) {
	pcilib_error("Can't change buffer size while grabbing");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if (ctx->size < 1) {
	pcilib_error("The buffer size is too small");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

    ctx->buffer_size = size;
    
    return 0;
}


int ipecamera_reset(pcilib_context_t *vctx) {
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
    err = pcilib_write_register_by_id(pcilib, control, 0x1e4);
    if (err) {
	pcilib_error("Error setting CMOSIS reset bit");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);

	// Remove Reset bit to CMOSIS
    err = pcilib_write_register_by_id(pcilib, control, 0x1e1);
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
    SET_REG(control_reg, IPECAMERA_IDLE);
    if (err) return err;
    
    usleep(10000);
    
    err = pcilib_read_register_by_id(pcilib, status, &value);
    if (err) {
	pcilib_error("Error reading status register");
	return err;
    }

    if (value != IPECAMERA_EXPECTED_STATUS) {
	pcilib_error("Unexpected value (%lx) of status register, expected %lx", value, IPECAMERA_EXPECTED_STATUS);
	return PCILIB_ERROR_VERIFY;
    }

    return 0;    
}

int ipecamera_start(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_event_callback_t cb, void *user) {
    int err = 0;
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = ctx->pcilib;
    pcilib_register_value_t value;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    if (ctx->started) {
	pcilib_error("IPECamera grabbing is already started");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

	// if left in FRAME_REQUEST mode
    SET_REG(control_reg, IPECAMERA_IDLE);
    usleep(IPECAMERA_SLEEP_TIME);
    CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);
    if (err) return err;


#ifdef IPECAMERA_DMA_ADDRESS
    SET_REG(packet_len_reg, IPECAMERA_DMA_PACKET_LENGTH);
    if (err) return err;
#endif /* IPECAMERA_DMA_ADDRESS */
    
    ctx->cb = cb;
    ctx->cb_user = user;
    
    ctx->event_id = 0;
    ctx->reported_id = 0;
    ctx->buf_ptr = 0;    

    ctx->dim.width = IPECAMERA_WIDTH;
    ctx->dim.height = IPECAMERA_HEIGHT; //GET_REG(lines_reg, lines);

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

#ifdef IPECAMERA_DMA_ADDRESS
    if (!err) {
	ctx->rdma = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_FROM_DEVICE, IPECAMERA_DMA_ADDRESS);
	if (ctx->rdma == PCILIB_DMA_ENGINE_INVALID) {
	    err = PCILIB_ERROR_NOTFOUND;
	    pcilib_error("The C2S channel of IPECamera DMA Engine (%u) is not found", IPECAMERA_DMA_ADDRESS);
	} else {
	    err = pcilib_start_dma(ctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	    if (err) {
		ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
		pcilib_error("Failed to initialize C2S channel of IPECamera DMA Engine (%u)", IPECAMERA_DMA_ADDRESS);
	    }
	}
    }
    
/*    
    if (!err) {
	ctx->wdma = pcilib_find_dma_by_addr(ctx->pcilib, PCILIB_DMA_TO_DEVICE, IPECAMERA_DMA_ADDRESS);
	if (ctx->wdma == PCILIB_DMA_ENGINE_INVALID) {
	    err = PCILIB_ERROR_NOTFOUND;
	    pcilib_error("The S2C channel of IPECamera DMA Engine (%u) is not found", IPECAMERA_DMA_ADDRESS);
	} else {
	    err = pcilib_start_dma(ctx->pcilib, ctx->wdma, PCILIB_DMA_FLAGS_DEFAULT);
	    if (err) {
		ctx->wdma = PCILIB_DMA_ENGINE_INVALID;
		pcilib_error("Failed to initialize S2C channel of IPECamera DMA Engine (%u)", IPECAMERA_DMA_ADDRESS);
	    }
	}
    }
*/    
#endif /* IPECAMERA_DMA_ADDRESS */
    
    if (err) {
	ipecamera_stop(vctx);
	return err;
    }
    
    ctx->started = 1;
    
    return 0;
}


int ipecamera_stop(pcilib_context_t *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    ctx->started = 0;   

#ifdef IPECAMERA_DMA_ADDRESS
    if (ctx->wdma != PCILIB_DMA_ENGINE_INVALID) {
	pcilib_stop_dma(ctx->pcilib, ctx->wdma, PCILIB_DMA_FLAGS_DEFAULT);
	ctx->wdma = PCILIB_DMA_ENGINE_INVALID;
    }

    if (ctx->rdma != PCILIB_DMA_ENGINE_INVALID) {
	pcilib_stop_dma(ctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
    }
#endif /* IPECAMERA_DMA_ADDRESS */

    if (ctx->buffer) {
	free(ctx->buffer);
	ctx->buffer = NULL;
    }
    
    if (ctx->cmask) {
	free(ctx->cmask);
	ctx->cmask = NULL;
    }


    ctx->event_id = 0;
    ctx->reported_id = 0;
    ctx->buf_ptr = 0; 

    return 0;
}


static int ipecamera_get_payload(ipecamera_t *ctx, ipecamera_pixel_t *pbuf, ipecamera_change_mask_t *cbuf, int line_req, pcilib_register_value_t size, ipecamera_payload_t *payload, pcilib_register_value_t *advance) {
    int i, j;
    int ppw;
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
    
    int pix;
    int pix_offset = 0;
    ipecamera_payload_t data;

#ifdef IPECAMERA_REORDER_CHANNELS
    channel = ipecamera_channel_order[channel];
#endif 
//    printf("payload, channel: %i, magick: %i, all: %lx\n", channel, header, info);

    //printf("channel[%x] = %x (line: %i, pixels: %i)\n", info, channel, line_req, pixels);
    CHECK_FLAG("payload header magick", header == 2, header);
    CHECK_FLAG("pixel size, only 10 bits are supported", bpp == 10, bpp);
    CHECK_FLAG("row number, should be %li", line == line_req, line, line_req);
    CHECK_FLAG("channel, limited by %li output channels", channel < IPECAMERA_MAX_CHANNELS, channel, IPECAMERA_MAX_CHANNELS);
    CHECK_FLAG("channel, duplicate entry for channel", ((*cbuf)&(1<<channel)) == 0, channel);

	// Fixing first lines bug
    if ((line < 2)&&(pixels == (IPECAMERA_PIXELS_PER_CHANNEL - 1))) {
	pix_offset = 1;
	pbuf[channel*IPECAMERA_PIXELS_PER_CHANNEL] = 0;
    } else {
	CHECK_FLAG("number of pixels, %li is expected", pixels == IPECAMERA_PIXELS_PER_CHANNEL, pixels, IPECAMERA_PIXELS_PER_CHANNEL);
    }
    
    bytes = pixels / 3;
    ppw = pixels - bytes * 3;
    if (ppw) ++bytes;

    CHECK_FLAG("payload data bytes, at least %i are expected", bytes < size, size, bytes);

    if (err) return err;

    for (i = 1, pix = pix_offset; i < bytes; i++) {
	data = payload[i];
        header = (data >> 30) & 0x03;	

        CHECK_FLAG("payload data magick", header == 3, header);
	if (err) return err;

        for (j = 0; j < 3; j++, pix++) {
            pbuf[channel*IPECAMERA_PIXELS_PER_CHANNEL + pix] = (data >> (10 * (2 - j))) & 0x3FF;
        }
    }

    data = payload[bytes];
    header = (data >> 30) & 0x03;

    CHECK_FLAG("payload data magick", header == 3, header);
    CHECK_FLAG("payload footer magick", (data&0x3FF) == 0x55, (data&0x3FF));
    if (err) return err;

    ppw = pixels%3;
    assert(ppw < 3);

    for (j = 0; j < ppw; j++, pix++) {
//	pbuf[channel*IPECAMERA_PIXELS_PER_CHANNEL + pix] = (data >> (10 * (ppw - j - 1))) & 0x3FF;
        pbuf[channel*IPECAMERA_PIXELS_PER_CHANNEL + pix] = (data >> (10 * (ppw - j))) & 0x3FF;
    }    

    *cbuf |= (1 << channel);
    *advance = bytes + 1;

    return 0;
}

static int ipecamera_parse_image(ipecamera_t *ctx, ipecamera_pixel_t *pbuf, ipecamera_change_mask_t *cbuf, int first_line, int n_lines, pcilib_register_value_t size, ipecamera_payload_t *linebuf) {
    int err = 0;
    pcilib_t *pcilib = ctx->pcilib;

    int line = first_line;
    pcilib_register_value_t pos, advance;

    if (size < 16) {
	pcilib_error("The payload is tool small, we should have at least 8 header dwords and 8 footer.");
	return PCILIB_ERROR_INVALID_DATA;
    }
    
    CHECK_VALUE(linebuf[0], 0x51111111);
    CHECK_VALUE(linebuf[1], 0x52222222);
    CHECK_VALUE(linebuf[2], 0x53333333);
    CHECK_VALUE(linebuf[3], 0x54444444);
    CHECK_VALUE(linebuf[4], 0x55555555);
    CHECK_VALUE(linebuf[5], 0x56666666);
    CHECK_VALUE(linebuf[6], 0x57777777);
    CHECK_VALUE(linebuf[7], 0x58888888);
    if (err) return err;

    pos = 8;
    size -= 16;

    while (size > 0) {
	err = ipecamera_get_payload(ctx, pbuf + line * ctx->dim.width, cbuf + line, line - first_line, size, linebuf + pos, &advance);
	if (err) return err;
	    
	pos += advance;
	size -= advance;
	
	if (cbuf[line] == ((1<<IPECAMERA_MAX_CHANNELS)-1)) ++line;
    }

    CHECK_FLAG("lines read, we expect to read exactly %li lines", line == (first_line + n_lines), line - first_line, n_lines);

    CHECK_VALUE(linebuf[pos  ], 0x0AAAAAAA);
    CHECK_VALUE(linebuf[pos+1], 0x0BBBBBBB);
    CHECK_VALUE(linebuf[pos+2], 0x0CCCCCCC);
    CHECK_VALUE(linebuf[pos+3], 0x0DDDDDDD);
    CHECK_VALUE(linebuf[pos+4], 0x0EEEEEEE);
    CHECK_VALUE(linebuf[pos+5], 0x0FFFFFFF);
    CHECK_VALUE(linebuf[pos+6], 0x00000000);
    CHECK_VALUE(linebuf[pos+7], 0x01111111);

    return err;
}

static int ipecamera_get_image(ipecamera_t *ctx) {
    int err;
    int i;
    int buf_ptr;
    size_t bytes_read;
    pcilib_t *pcilib = ctx->pcilib;

    int num_lines;
    const int max_lines = 1;//1088;//IPECAMERA_MAX_LINES;
    const size_t line_size = (IPECAMERA_MAX_CHANNELS * (2 + IPECAMERA_PIXELS_PER_CHANNEL / 3));
    const size_t hf_size = 16;
    const size_t max_size = hf_size + max_lines * line_size;
    const size_t dma_packet_len = IPECAMERA_DMA_PACKET_LENGTH / sizeof(ipecamera_payload_t);
    size_t max_packet_size;

    pcilib_register_value_t ptr, size, pos, advance, value;

    ipecamera_payload_t *linebuf;
    
    if (max_size%dma_packet_len) max_packet_size = max_size + dma_packet_len - (max_size%dma_packet_len);
    else max_packet_size = max_size;

    max_packet_size += 4096;	// Some extra data?
    
    
    linebuf = (ipecamera_payload_t*)malloc(max_packet_size * sizeof(ipecamera_payload_t));
    if (!linebuf) return PCILIB_ERROR_MEMORY;

#ifdef IPECAMERA_WRITE_RAW
    FILE *f = fopen("raw/image.raw", "w");
    if (f) fclose(f);
#endif

    //atomic    
    buf_ptr = ctx->buf_ptr;
    if (ctx->buf_ptr++ == ctx->buffer_size) ctx->buf_ptr = 0;
    if (ctx->event_id++ == 0) ctx->event_id = 1;

    //const size_t image_size = ctx->dim.width * ctx->dim.height;
    //memset(ctx->buffer + buf_ptr * image_size, 0, image_size * sizeof(ipecamera_pixel_t));
    memset(ctx->cmask + buf_ptr * ctx->dim.height, 0, ctx->dim.height * sizeof(ipecamera_change_mask_t));


    for (i = 0; i < ctx->dim.height; i += max_lines) {
	num_lines = ctx->dim.height - i;
	if (num_lines > max_lines)  num_lines = max_lines;
    
        SET_REG(n_lines_reg, num_lines);
	SET_REG(line_reg, i);

	SET_REG(control_reg, IPECAMERA_FRAME_REQUEST);
	usleep(IPECAMERA_SLEEP_TIME);
	CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);
	SET_REG(control_reg, IPECAMERA_IDLE);

#ifndef IPECAMERA_DMA_ADDRESS	
	GET_REG(start_reg, ptr);
	GET_REG(end_reg, size);

	size -= ptr;
	size *= 2;
	
	CHECK_FLAG("data size", (size > 0)&&(size <= max_size), size);
#endif /* IPECAMERA_DMA_ADDRESS */

	
	if (err) break;
	
	SET_REG(control_reg, IPECAMERA_READOUT);
		
#ifdef IPECAMERA_DMA_ADDRESS
	size = 0;
	do {
	    err = pcilib_read_dma(ctx->pcilib, ctx->rdma, 0, max_packet_size * sizeof(ipecamera_payload_t) - size, ((uint8_t*)linebuf) + size, &bytes_read);
	    size += bytes_read;
//	    printf("%lu %lu\n", bytes_read, size);
	} while ((err == 0)&&(size < max_packet_size * sizeof(ipecamera_payload_t)));

#ifdef DEBUG_HARDWARE
	uint32_t regval;
	printf("===========Lines: %i - %i =========================\n", i, i + num_lines - 1);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9050", &regval);
	printf("Status1: %i 0x%lx\n", err, regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9080", &regval);
	printf("Start address: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9090", &regval);
	printf("End address: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9100", &regval);
	printf("Status2: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9110", &regval);
	printf("Status3: %i 0x%lx\n", err,  regval);
	err = pcilib_read_register(ctx->pcilib, NULL, "reg9160", &regval);
	printf("Add_rd_ddr: %i 0x%lx\n", err, regval);
#endif /* DEBUG_HARDWARE */

	if (err) {
	    if (err == PCILIB_ERROR_TIMEOUT) {
		if (size > 0) err = 0;
		else {
//		    pcilib_error("There is no data received from IPE Camera");
		    pcilib_warning("There is no data received from IPE Camera for lines: %i to %i", i, i + num_lines - 1);
		    err = 0;
		    SET_REG(control_reg, IPECAMERA_IDLE);
		    continue;
		}
	    } else pcilib_error("DMA read from IPE Camera have failed");
	} else if (!size) {
		pcilib_warning("There is no data received from IPE Camera for lines: %i to %i", i, i + num_lines - 1);
		SET_REG(control_reg, IPECAMERA_IDLE);
		continue;
	}

        pcilib_warning("Reading lines %i to %i: got %i bytes from DMA", i, i + num_lines - 1, size);  
#else /* IPECAMERA_DMA_ADDRESS */
        pcilib_warning("Reading lines %i to %i: %i %i", i, i + num_lines - 1, ptr, size);  
	pcilib_datacpy(linebuf, ctx->data + ptr, sizeof(ipecamera_payload_t), size, pcilib_model[PCILIB_MODEL_IPECAMERA].endianess);
#endif /* IPECAMERA_DMA_ADDRESS */

	SET_REG(control_reg, IPECAMERA_IDLE);
	usleep(IPECAMERA_SLEEP_TIME);
	CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);

	if (err) break;

#ifdef IPECAMERA_WRITE_RAW
	char fname[256];
	sprintf(fname, "raw/line%04i", i);
	FILE *f = fopen(fname, "w");
	if (f) {
#ifdef IPECAMERA_DMA_ADDRESS
	    (void)fwrite(linebuf, sizeof(ipecamera_payload_t), size / sizeof(ipecamera_payload_t), f);
#else /* IPECAMERA_DMA_ADDRESS */
	    (void)fwrite(linebuf, sizeof(ipecamera_payload_t), size, f);
#endif /* IPECAMERA_DMA_ADDRESS */
	    fclose(f);
	}
#endif

#ifdef IPECAMERA_DMA_ADDRESS
	if (size < (hf_size + max_lines * line_size) * sizeof(ipecamera_payload_t)) {
	    pcilib_error("We are expecting at least %zu bytes, but only %lu are read", (hf_size + num_lines * line_size)*sizeof(ipecamera_payload_t), size);
	    err = PCILIB_ERROR_INVALID_DATA;
	}

	bytes_read = size;
	size = hf_size + max_lines * line_size;
#endif /* IPECAMERA_DMA_ADDRESS */


        err = ipecamera_parse_image(ctx, ctx->buffer + buf_ptr * ctx->dim.width * ctx->dim.height,  ctx->cmask + buf_ptr * ctx->dim.height, i, num_lines, size, linebuf);
	if (err) break;
	
#ifdef IPECAMERA_WRITE_RAW
        f = fopen("raw/image.raw", "a+");
        if (f) {
            fwrite(ctx->buffer + buf_ptr * ctx->dim.width * ctx->dim.height + i * ctx->dim.width, sizeof(ipecamera_pixel_t), num_lines * ctx->dim.width, f);
    	    fclose(f);
	}
#endif
    }

    free(linebuf);

    return err;
}


int ipecamera_trigger(pcilib_context_t *vctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    int err;
    pcilib_t *pcilib;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (!ctx->started) {
	pcilib_error("Can't trigger while not grabbing is not started");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    err = ipecamera_get_image(ctx);
    if (!err) {
	if (ctx->cb) {
	    err = ctx->cb(event, ctx->event_id, ctx->cb_user);
	    ctx->reported_id = ctx->event_id;
	}
    }

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

pcilib_event_id_t ipecamera_next_event(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_timeout_t timeout) {
    int buf_ptr;
    pcilib_event_id_t reported;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_EVENT_ID_INVALID;
    }

    if (!ctx->started) {
	pcilib_error("IPECamera is not in grabbing state");
	return PCILIB_EVENT_ID_INVALID;
    }

    if ((!ctx->event_id)||(ctx->reported_id == ctx->event_id)) {
	if (timeout) {
	    // We should wait here for the specified timeout
	}
	return PCILIB_EVENT_ID_INVALID;
    }

	// We had an overflow in event counting
    if (ctx->reported_id > ctx->event_id) {
	do {
	    if (++ctx->reported_id == 0) ctx->reported_id = 1;
	} while (ipecamera_resolve_event_id(ctx, ctx->reported_id) < 0);
    } else {
	if ((ctx->event_id - ctx->reported_id) > ctx->buffer_size) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1);
	else ++ctx->reported_id;
    }
    
    return ctx->reported_id;
}

void* ipecamera_get(pcilib_context_t *vctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size) {
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



int ipecamera_return(pcilib_context_t *vctx, pcilib_event_id_t event_id) {
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
