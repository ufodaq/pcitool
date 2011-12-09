#define _IPECAMERA_IMAGE_C
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

#include <ufodecode.h>

#include "../tools.h"
#include "../error.h"

#include "pcilib.h"

#include "model.h"
#include "event.h"
#include "image.h"

#include "dma/nwl_dma.h"

#ifdef IPECAMERA_DEBUG
#include "dma/nwl.h"
#endif /* IPECAMERA_DEBUG */


#define IPECAMERA_BUG_EXTRA_DATA
#define IPECAMERA_BUG_MULTIFRAME_PACKETS
#define IPECAMERA_BUG_INCOMPLETE_PACKETS

#define IPECAMERA_DEFAULT_BUFFER_SIZE 64  	//**< should be power of 2 */
#define IPECAMERA_RESERVE_BUFFERS 2		//**< Return Frame is Lost error, if requested frame will be overwritten after specified number of frames
#define IPECAMERA_SLEEP_TIME 250000 		//**< Michele thinks 250 should be enough, but reset failing in this case */
#define IPECAMERA_NEXT_FRAME_DELAY 1000 	//**< Michele requires 30000 to sync between End Of Readout and next Frame Req */
#define IPECAMERA_WAIT_FRAME_RCVD_TIME 0 	//**< by Uros ,wait 6 ms */
#define IPECAMERA_NOFRAME_SLEEP 100

#define IPECAMERA_MAX_LINES 1088
#define IPECAMERA_EXPECTED_STATUS 0x08409FFFF
#define IPECAMERA_END_OF_SEQUENCE 0x1F001001

#define IPECAMERA_MAX_CHANNELS 16
#define IPECAMERA_PIXELS_PER_CHANNEL 128
#define IPECAMERA_WIDTH (IPECAMERA_MAX_CHANNELS * IPECAMERA_PIXELS_PER_CHANNEL)

/*
#define IPECAMERA_HEIGHT 1088
#if IPECAMERA_HEIGHT < IPECAMERA_MAX_LINES
# undef IPECAMERA_MAX_LINES
# define IPECAMERA_MAX_LINES IPECAMERA_HEIGHT
#endif 
*/

//#define IPECAMERA_MEMORY 

#define IPECAMERA_FRAME_REQUEST 		0x1E9
#define IPECAMERA_READOUT_FLAG			0x200
#define IPECAMERA_READOUT			0x3E1
#define IPECAMERA_IDLE 				0x1E1
#define IPECAMERA_START_INTERNAL_STIMULI 	0x1F1


typedef uint32_t ipecamera_payload_t;

typedef struct {
    pcilib_event_id_t evid;
    struct timeval timestamp;
} ipecamera_autostop_t;

struct ipecamera_s {
    pcilib_context_t event;
    ufo_decoder ipedec;

    char *data;
    ipecamera_pixel_t *image;
    size_t size;

    pcilib_event_callback_t cb;
    void *cb_user;

    pcilib_event_id_t event_id;
    pcilib_event_id_t reported_id;
    
    pcilib_dma_engine_t rdma, wdma;

    pcilib_register_t packet_len_reg;
    pcilib_register_t control_reg, status_reg;
    pcilib_register_t start_reg, end_reg;
    pcilib_register_t n_lines_reg;
    uint16_t line_reg;
    pcilib_register_t exposure_reg;
    pcilib_register_t flip_reg;

    int started;		/**< Camera is in grabbing mode (start function is called) */
    int streaming;		/**< Camera is in streaming mode (we are within stream call) */
    int parse_data;		/**< Indicates if some processing of the data is required, otherwise only rawdata_callback will be called */

    int run_reader;		/**< Instructs the reader thread to stop processing */
    int run_streamer;		/**< Indicates request to stop streaming events and can be set by reader_thread upon exit or by user request */
    ipecamera_autostop_t autostop;

    struct timeval autostop_time;

    size_t buffer_size;		/**< How many images to store */
    size_t buffer_pos;		/**< Current image offset in the buffer, due to synchronization reasons should not be used outside of reader_thread */
    size_t cur_size;		/**< Already written part of data in bytes */
    size_t raw_size;		/**< Size of raw data in bytes */
    size_t full_size;		/**< Size of raw data including the padding */
    size_t padded_size;		/**< Size of buffer for raw data, including the padding for performance */
    
    size_t image_size;		/**< Size of a single image in bytes */
    
    int width, height;

    
//    void *raw_buffer;
    void *buffer;
    ipecamera_change_mask_t *cmask;
    ipecamera_event_info_t *frame_info;
    

    ipecamera_image_dimensions_t dim;

    pthread_t rthread;
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
	
	ctx->buffer_size = IPECAMERA_DEFAULT_BUFFER_SIZE;
	ctx->dim.bpp = sizeof(ipecamera_pixel_t) * 8;

	    // We need DMA engine initialized to resolve DMA registers
//	FIND_REG(packet_len_reg, "fpga", "xrawdata_packet_length");
	
	FIND_REG(status_reg, "fpga", "status");
	FIND_REG(control_reg, "fpga", "control");
	FIND_REG(start_reg, "fpga", "start_address");
	FIND_REG(end_reg, "fpga", "end_address");

	FIND_REG(n_lines_reg, "cmosis", "number_lines");
	FIND_REG(line_reg, "cmosis", "start1");
	FIND_REG(exposure_reg, "cmosis", "exp_time");
	FIND_REG(flip_reg, "cmosis", "image_flipping");

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
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	free(ctx);
    }
}

pcilib_dma_context_t *ipecamera_init_dma(pcilib_context_t *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    
    pcilib_model_description_t *model_info = pcilib_get_model_description(vctx->pcilib);
    if ((!model_info->dma_api)||(!model_info->dma_api->init)) {
	pcilib_error("The DMA engine is not configured in model");
	return NULL;
    }


#ifdef IPECAMERA_DMA_R3
    return model_info->dma_api->init(vctx->pcilib, PCILIB_NWL_MODIFICATION_IPECAMERA, NULL);
#else
    return model_info->dma_api->init(vctx->pcilib, PCILIB_DMA_MODIFICATION_DEFAULT, NULL);
#endif
}


int ipecamera_set_buffer_size(ipecamera_t *ctx, int size) {
    if (ctx->started) {
	pcilib_error("Can't change buffer size while grabbing");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if (size < 2) {
	pcilib_error("The buffer size is too small");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if (((size^(size-1)) < size) < size) {
	pcilib_error("The buffer size is not power of 2");
    }
    
    ctx->buffer_size = size;
    
    return 0;
}

int ipecamera_reset(pcilib_context_t *vctx) {
    int err = 0;
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;

    pcilib_register_t control, status;
    pcilib_register_value_t value;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    pcilib = vctx->pcilib;
    control = ctx->control_reg;
    status = ctx->status_reg;

	// Set Reset bit to CMOSIS
    err = pcilib_write_register_by_id(pcilib, control, 0x1e4);
    if (err) {
	pcilib_error("Error setting FPGA reset bit");
	return err;
    }
    usleep(IPECAMERA_SLEEP_TIME);

	// Remove Reset bit to CMOSIS
    err = pcilib_write_register_by_id(pcilib, control, 0x1e1);
    if (err) {
	pcilib_error("Error reseting FPGA reset bit");
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
    err = pcilib_write_register_by_id(pcilib, control, IPECAMERA_IDLE);
    if (err) {
	pcilib_error("Error bringing FPGA in default mode");
	return err;
    }

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

// DS: Currently, on event_id overflow we are assuming the buffer is lost
static int ipecamera_resolve_event_id(ipecamera_t *ctx, pcilib_event_id_t evid) {
    pcilib_event_id_t diff;

    if (evid > ctx->event_id) {
	diff = (((pcilib_event_id_t)-1) - ctx->event_id) + evid;
	if (diff >= ctx->buffer_size) return -1;
    } else {
	diff = ctx->event_id - evid;
        if (diff >= ctx->buffer_size) return -1;
    }
    
	// DS: Request buffer_size to be power of 2 and replace to shifts (just recompute in set_buffer_size)
    return (evid - 1) % ctx->buffer_size;
}

static inline int ipecamera_new_frame(ipecamera_t *ctx) {
    ctx->frame_info[ctx->buffer_pos].raw_size = ctx->cur_size;

    if (ctx->cur_size < ctx->raw_size) ctx->frame_info[ctx->buffer_pos].info.flags |= PCILIB_EVENT_INFO_FLAG_BROKEN;
    
    ctx->buffer_pos = (++ctx->event_id) % ctx->buffer_size;
    ctx->cur_size = 0;

    ctx->frame_info[ctx->buffer_pos].info.type = PCILIB_EVENT0;
    ctx->frame_info[ctx->buffer_pos].info.flags = 0;
    ctx->frame_info[ctx->buffer_pos].image_ready = 0;

    if ((ctx->event_id == ctx->autostop.evid)&&(ctx->event_id)) {
	ctx->run_reader = 0;
	return 1;
    }
	
    if (pcilib_check_deadline(&ctx->autostop.timestamp, PCILIB_DMA_TIMEOUT)) {
	ctx->run_reader = 0;
	return 1;
    }
    
    return 0;
}

static uint32_t frame_magic[6] = { 0x51111111, 0x52222222, 0x53333333, 0x54444444, 0x55555555, 0x56666666 };

static int ipecamera_data_callback(void *user, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    int eof = 0;

#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
    size_t extra_data = 0;
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */

    ipecamera_t *ctx = (ipecamera_t*)user;

    if (!ctx->cur_size) {
#if defined(IPECAMERA_BUG_INCOMPLETE_PACKETS)||defined(IPECAMERA_BUG_MULTIFRAME_PACKETS)
	size_t startpos;
	for (startpos = 0; (startpos + 8) < bufsize; startpos++) {
	    if (!memcmp(buf + startpos, frame_magic, sizeof(frame_magic))) break;
	}
	
	if (startpos) {
	    buf += startpos;
	    bufsize -= startpos;
	}
#endif /* IPECAMERA_BUG_INCOMPLETE_PACKETS */

	if ((bufsize >= 8)&&(!memcmp(buf, frame_magic, sizeof(frame_magic)))) {
/*
		// Not implemented in hardware yet
	    ctx->frame_info[ctx->buffer_pos].info.seqnum = ((uint32_t*)buf)[6] & 0xF0000000;
	    ctx->frame_info[ctx->buffer_pos].info.offset = ((uint32_t*)buf)[7] & 0xF0000000;
*/
	    gettimeofday(&ctx->frame_info[ctx->buffer_pos].info.timestamp, NULL);
	} else {
//	    pcilib_warning("Frame magic is not found, ignoring broken data...");
	    return PCILIB_STREAMING_CONTINUE;
	}
    }

#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
    if (ctx->cur_size + bufsize > ctx->raw_size) {
        size_t need;
	
	for (need = ctx->raw_size - ctx->cur_size; need < bufsize; need += sizeof(uint32_t)) {
	    if (*(uint32_t*)(buf + need) == frame_magic[0]) break;
	}
	
	if (need < bufsize) {
	    extra_data = bufsize - need;
	    //bufsize = need;
	    eof = 1;
	}
	
	    // just rip of padding
	bufsize = ctx->raw_size - ctx->cur_size;
    }
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */

    if (ctx->parse_data) {
	if (ctx->cur_size + bufsize > ctx->full_size) {
    	    pcilib_error("Unexpected event data, we are expecting at maximum (%zu) bytes, but (%zu) already read", ctx->full_size, ctx->cur_size + bufsize);
	    return -PCILIB_ERROR_TOOBIG;
	}

        memcpy(ctx->buffer + ctx->buffer_pos * ctx->padded_size +  ctx->cur_size, buf, bufsize);
    }

    ctx->cur_size += bufsize;
//    printf("%i: %i %i\n", ctx->buffer_pos, ctx->cur_size, bufsize);

    if (ctx->cur_size >= ctx->full_size) eof = 1;

    if (ctx->event.params.rawdata.callback) {
	ctx->event.params.rawdata.callback(ctx->event_id, (pcilib_event_info_t*)(ctx->frame_info + ctx->buffer_pos), (eof?PCILIB_EVENT_FLAG_EOF:PCILIB_EVENT_FLAGS_DEFAULT), bufsize, buf, ctx->event.params.rawdata.user);
    }
    
    if (eof) {
	if (ipecamera_new_frame(ctx)) {
	    return PCILIB_STREAMING_STOP;
	}
	
#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
	if (extra_data) {
	    return ipecamera_data_callback(user, flags, extra_data, buf + bufsize);
	}
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */
    }

    return PCILIB_STREAMING_REQ_FRAGMENT;
}

static void *ipecamera_reader_thread(void *user) {
    int err;
    ipecamera_t *ctx = (ipecamera_t*)user;
    
    while (ctx->run_reader) {
	err = pcilib_stream_dma(ctx->event.pcilib, ctx->rdma, 0, 0, PCILIB_DMA_FLAG_MULTIPACKET, PCILIB_DMA_TIMEOUT, &ipecamera_data_callback, user);
	if (err) {
	    if (err == PCILIB_ERROR_TIMEOUT) {
		if (ctx->cur_size > ctx->raw_size) ipecamera_new_frame(ctx);
#ifdef IPECAMERA_BUG_INCOMPLETE_PACKETS
		else if (ctx->cur_size > 0) ipecamera_new_frame(ctx);
#endif /* IPECAMERA_BUG_INCOMPLETE_PACKETS */
		if (pcilib_check_deadline(&ctx->autostop.timestamp, PCILIB_DMA_TIMEOUT)) {
		    ctx->run_reader = 0;
		    break;
		}
		usleep(IPECAMERA_NOFRAME_SLEEP);
	    } else pcilib_error("DMA error while reading IPECamera frames, error: %i", err);
	} else printf("no error\n");

	//usleep(1000);
    }
    
    ctx->run_streamer = 0;
    
    if (ctx->cur_size)
	pcilib_error("partialy read frame after stop signal, %zu bytes in the buffer", ctx->cur_size);

    return NULL;
}

int ipecamera_start(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_event_flags_t flags) {
    int err = 0;
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;
    pcilib_register_value_t value;
    
    const size_t chan_size = (2 + IPECAMERA_PIXELS_PER_CHANNEL / 3) * sizeof(ipecamera_payload_t);
    const size_t line_size = (IPECAMERA_MAX_CHANNELS * chan_size);
    const size_t header_size = 8 * sizeof(ipecamera_payload_t);
    const size_t footer_size = 8 * sizeof(ipecamera_payload_t);
    size_t raw_size;
    size_t padded_blocks;
    
    pthread_attr_t attr;
    struct sched_param sched;
    
    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    if (ctx->started) {
	pcilib_error("IPECamera grabbing is already started");
	return PCILIB_ERROR_INVALID_REQUEST;
    }


	// Allow readout and clean the FRAME_REQUEST mode if set for some reason
    SET_REG(control_reg, IPECAMERA_IDLE|IPECAMERA_READOUT_FLAG);
    usleep(IPECAMERA_SLEEP_TIME);
    CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);
    if (err) return err;


    ctx->event_id = 0;
    ctx->reported_id = 0;
    ctx->buffer_pos = 0;
    ctx->parse_data = (flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY)?0:1;
    ctx->cur_size = 0;

    ctx->dim.width = IPECAMERA_WIDTH;
    GET_REG(n_lines_reg, ctx->dim.height);
    
    raw_size = header_size + ctx->dim.height * line_size + footer_size;
    padded_blocks = raw_size / IPECAMERA_DMA_PACKET_LENGTH + ((raw_size % IPECAMERA_DMA_PACKET_LENGTH)?1:0);
    
    ctx->image_size = ctx->dim.width * ctx->dim.height;
    ctx->raw_size = raw_size;
    ctx->full_size = padded_blocks * IPECAMERA_DMA_PACKET_LENGTH;

#ifdef IPECAMERA_BUG_EXTRA_DATA
    ctx->full_size += 8;
    padded_blocks ++;
#endif /* IPECAMERA_BUG_EXTRA_DATA */

    ctx->padded_size = padded_blocks * IPECAMERA_DMA_PACKET_LENGTH;

    ctx->buffer = malloc(ctx->padded_size * ctx->buffer_size);
    if (!ctx->buffer) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate ring buffer (%lu bytes)", ctx->padded_size * ctx->buffer_size);
	return err;
    }

    ctx->image = (ipecamera_pixel_t*)malloc(ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
    if (!ctx->image) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate image buffer (%lu bytes)", ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
	return err;
    }

    ctx->cmask = malloc(ctx->dim.height * ctx->buffer_size * sizeof(ipecamera_change_mask_t));
    if (!ctx->cmask) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate change-mask buffer");
	return err;
    }

    ctx->frame_info = malloc(ctx->buffer_size * sizeof(ipecamera_event_info_t));
    if (!ctx->frame_info) {
	err = PCILIB_ERROR_MEMORY;
	pcilib_error("Unable to allocate frame-info buffer");
	return err;
    }
    
    ctx->ipedec = ufo_decoder_new(ctx->dim.height, ctx->dim.width, NULL, 0);
    if (!ctx->ipedec) {
	pcilib_error("Unable to initialize IPECamera decoder library");
	return PCILIB_ERROR_FAILED;
    }

    if (!err) {
	ctx->rdma = pcilib_find_dma_by_addr(vctx->pcilib, PCILIB_DMA_FROM_DEVICE, IPECAMERA_DMA_ADDRESS);
	if (ctx->rdma == PCILIB_DMA_ENGINE_INVALID) {
	    err = PCILIB_ERROR_NOTFOUND;
	    pcilib_error("The C2S channel of IPECamera DMA Engine (%u) is not found", IPECAMERA_DMA_ADDRESS);
	} else {
	    err = pcilib_start_dma(vctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	    if (err) {
		ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
		pcilib_error("Failed to initialize C2S channel of IPECamera DMA Engine (%u)", IPECAMERA_DMA_ADDRESS);
	    }
	}
    }
    
/*    
    if (!err) {
	ctx->wdma = pcilib_find_dma_by_addr(vctx->pcilib, PCILIB_DMA_TO_DEVICE, IPECAMERA_DMA_ADDRESS);
	if (ctx->wdma == PCILIB_DMA_ENGINE_INVALID) {
	    err = PCILIB_ERROR_NOTFOUND;
	    pcilib_error("The S2C channel of IPECamera DMA Engine (%u) is not found", IPECAMERA_DMA_ADDRESS);
	} else {
	    err = pcilib_start_dma(vctx->pcilib, ctx->wdma, PCILIB_DMA_FLAGS_DEFAULT);
	    if (err) {
		ctx->wdma = PCILIB_DMA_ENGINE_INVALID;
		pcilib_error("Failed to initialize S2C channel of IPECamera DMA Engine (%u)", IPECAMERA_DMA_ADDRESS);
	    }
	}
    }
*/    

/*
    SET_REG(packet_len_reg, IPECAMERA_DMA_PACKET_LENGTH);
    if (err) return err;
*/

	// Clean DMA
    err = pcilib_skip_dma(vctx->pcilib, ctx->rdma);
    if (err) {
	pcilib_error("Can't start grabbing, device continuously writes unexpected data using DMA engine");
	return err;
    }

    if (err) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	return err;
    }

    if (vctx->params.autostop.duration) {
	gettimeofday(&ctx->autostop.timestamp, NULL);
	ctx->autostop.timestamp.tv_usec += vctx->params.autostop.duration % 1000000;
	if (ctx->autostop.timestamp.tv_usec > 999999) {
	    ctx->autostop.timestamp.tv_sec += 1 + vctx->params.autostop.duration / 1000000;
	    ctx->autostop.timestamp.tv_usec -= 1000000;
	} else {
	    ctx->autostop.timestamp.tv_sec += vctx->params.autostop.duration / 1000000;
	}
    }
    
    if (vctx->params.autostop.max_events) {
	ctx->autostop.evid = vctx->params.autostop.max_events;
    }
    
    ctx->started = 1;
    ctx->run_reader = 1;
    
    pthread_attr_init(&attr);

    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
	pcilib_warning("Can't schedule a real-time thread, you may consider running as root");
    } else {
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;	// Let 1 priority for something really critcial
	pthread_attr_setschedparam(&attr, &sched);
    }
    
    if (pthread_create(&ctx->rthread, &attr, &ipecamera_reader_thread, (void*)ctx)) {
	ctx->started = 0;
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	err = PCILIB_ERROR_THREAD;
    }
    
    pthread_attr_destroy(&attr);    

    return err;
}


int ipecamera_stop(pcilib_context_t *vctx, pcilib_event_flags_t flags) {
    int err;
    void *retcode;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (flags&PCILIB_EVENT_FLAG_STOP_ONLY) {
	ctx->run_reader = 0;
	return 0;
    }
    
    if (ctx->started) {
	ctx->run_reader = 0;
	err = pthread_join(ctx->rthread, &retcode);
	if (err) pcilib_error("Error joining the reader thread");
    }

    if (ctx->wdma != PCILIB_DMA_ENGINE_INVALID) {
	pcilib_stop_dma(vctx->pcilib, ctx->wdma, PCILIB_DMA_FLAGS_DEFAULT);
	ctx->wdma = PCILIB_DMA_ENGINE_INVALID;
    }

    if (ctx->rdma != PCILIB_DMA_ENGINE_INVALID) {
	pcilib_stop_dma(vctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
    }
    
    if (ctx->ipedec) {
	ufo_decoder_free(ctx->ipedec);
	ctx->ipedec = NULL;
    }

    if (ctx->frame_info) {
	free(ctx->frame_info);
	ctx->frame_info = NULL;
    }

    if (ctx->cmask) {
	free(ctx->cmask);
	ctx->cmask = NULL;
    }

    if (ctx->image) {
	free(ctx->image);
	ctx->image = NULL;
    }

    if (ctx->buffer) {
	free(ctx->buffer);
	ctx->buffer = NULL;
    }
    

    memset(&ctx->autostop, 0, sizeof(ipecamera_autostop_t));

    ctx->event_id = 0;
    ctx->reported_id = 0;
    ctx->buffer_pos = 0; 
    ctx->started = 0;

    return 0;
}


int ipecamera_trigger(pcilib_context_t *vctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    int err = 0;
    pcilib_register_value_t value;

    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (!ctx->started) {
	pcilib_error("Can't trigger while grabbing is not started");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

    SET_REG(control_reg, IPECAMERA_FRAME_REQUEST|IPECAMERA_READOUT_FLAG);
    usleep(IPECAMERA_WAIT_FRAME_RCVD_TIME);
    CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);
    SET_REG(control_reg, IPECAMERA_IDLE|IPECAMERA_READOUT_FLAG);


	// DS: Just measure when next trigger is allowed instead and wait in the beginning
    usleep(IPECAMERA_NEXT_FRAME_DELAY);  // minimum delay between End Of Readout and next Frame Req

    return 0;
}

int ipecamera_stream(pcilib_context_t *vctx, pcilib_event_callback_t callback, void *user) {
    int err = 0;
    int do_stop = 0;
    pcilib_event_id_t reported;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    size_t events = 0;

    struct timeval stream_stop;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    ctx->streaming = 1;
    ctx->run_streamer = 1;

    if (!ctx->started) {
	err = ipecamera_start(vctx, PCILIB_EVENTS_ALL, PCILIB_EVENT_FLAGS_DEFAULT);
	if (err) {
	    ctx->streaming = 0;
	    pcilib_error("IPECamera is not in grabbing state");
	    return PCILIB_ERROR_INVALID_STATE;
	}
	
	do_stop = 1;
    }
    
	// This loop iterates while the generation
    while ((ctx->run_streamer)||(ctx->reported_id != ctx->event_id)) {
	while (ctx->reported_id != ctx->event_id) {
	    if ((ctx->event_id - ctx->reported_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1) - IPECAMERA_RESERVE_BUFFERS;
	    else ++ctx->reported_id;

	    callback(ctx->reported_id, (pcilib_event_info_t*)(ctx->frame_info + ((ctx->reported_id-1)%ctx->buffer_size)), user);
	}
	usleep(IPECAMERA_NOFRAME_SLEEP);
    }

    ctx->streaming = 0;

    if (do_stop) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
    }
    

    return err;
}

int ipecamera_next_event(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_timeout_t timeout, pcilib_event_id_t *evid, pcilib_event_info_t **info) {
    struct timeval tv;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (!ctx->started) {
	pcilib_error("IPECamera is not in grabbing mode");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

    if (ctx->reported_id == ctx->event_id) {
	if (timeout) {
	    pcilib_calc_deadline(&tv, timeout);
	    
	    while ((pcilib_calc_time_to_deadline(&tv) > 0)&&(ctx->reported_id == ctx->event_id))
		usleep(IPECAMERA_NOFRAME_SLEEP);
	}
	
	if (ctx->reported_id == ctx->event_id) return PCILIB_ERROR_TIMEOUT;
    }

    if ((ctx->event_id - ctx->reported_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1) - IPECAMERA_RESERVE_BUFFERS;
    else ++ctx->reported_id;

    if (evid) *evid = ctx->reported_id;
    if (info) *info = (pcilib_event_info_t*)(ctx->frame_info + ((ctx->reported_id-1)%ctx->buffer_size));
    
    return 0;
}


inline static int ipecamera_decode_frame(ipecamera_t *ctx, pcilib_event_id_t event_id) {
    int err;
    uint32_t tmp;
    uint16_t *pixels;
    
    int buf_ptr = (event_id - 1) % ctx->buffer_size;
    
    if (!ctx->frame_info[buf_ptr].image_ready) {
	if (ctx->frame_info[buf_ptr].info.flags&PCILIB_EVENT_INFO_FLAG_BROKEN) return PCILIB_ERROR_INVALID_DATA;
	
	ufo_decoder_set_raw_data(ctx->ipedec, ctx->buffer + buf_ptr * ctx->padded_size, ctx->raw_size);
		
	pixels = ctx->image + buf_ptr * ctx->image_size;
	memset(ctx->cmask + ctx->buffer_pos * ctx->dim.height, 0, ctx->dim.height * sizeof(ipecamera_change_mask_t));
	err = ufo_decoder_get_next_frame(ctx->ipedec, &pixels, &tmp, &tmp, ctx->cmask + ctx->buffer_pos * ctx->dim.height);
	if (err) return PCILIB_ERROR_FAILED;
	    
	ctx->frame_info[buf_ptr].image_ready = 1;

	if (ipecamera_resolve_event_id(ctx, event_id) < 0) {
	    ctx->frame_info[buf_ptr].image_ready = 0;
	    return PCILIB_ERROR_TIMEOUT;
	}
    }
    
    return 0;
}

void* ipecamera_get(pcilib_context_t *vctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void *data) {
    int err;
    int buf_ptr;
    size_t raw_size;
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    uint16_t *pixels;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return NULL;
    }
    

    buf_ptr = ipecamera_resolve_event_id(ctx, event_id);
    if (buf_ptr < 0) return NULL;
    
    switch ((ipecamera_data_type_t)data_type) {
	case IPECAMERA_RAW_DATA:
	    raw_size = ctx->frame_info[buf_ptr].raw_size;
	    if (data) {
		if ((!size)||(*size < raw_size)) return NULL;
		memcpy(data, ctx->buffer + buf_ptr * ctx->padded_size, raw_size);
		if (ipecamera_resolve_event_id(ctx, event_id) < 0) return NULL;
		*size = raw_size;
		return data;
	    }
	    if (size) *size = raw_size;
	    return ctx->buffer + buf_ptr * ctx->padded_size;
	case IPECAMERA_IMAGE_DATA:
	    err = ipecamera_decode_frame(ctx, event_id);
	    if (err) return NULL;

	    if (data) {
		if ((!size)||(*size < ctx->image_size * sizeof(ipecamera_pixel_t))) return NULL;
		memcpy(data, ctx->image + buf_ptr * ctx->image_size, ctx->image_size * sizeof(ipecamera_pixel_t));
		if (ipecamera_resolve_event_id(ctx, event_id) < 0) return NULL;
		*size =  ctx->image_size * sizeof(ipecamera_pixel_t);
		return data;
	    }
	
	    if (size) *size = ctx->image_size * sizeof(ipecamera_pixel_t);
	    return ctx->image + buf_ptr * ctx->image_size;
	case IPECAMERA_CHANGE_MASK:
	    err = ipecamera_decode_frame(ctx, event_id);
	    if (err) return NULL;

	    if (data) {
		if ((!size)||(*size < ctx->dim.height * sizeof(ipecamera_change_mask_t))) return NULL;
		memcpy(data, ctx->image + buf_ptr * ctx->dim.height, ctx->dim.height * sizeof(ipecamera_change_mask_t));
		if (ipecamera_resolve_event_id(ctx, event_id) < 0) return NULL;
		*size =  ctx->dim.height * sizeof(ipecamera_change_mask_t);

		return data;
	    }

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



int ipecamera_return(pcilib_context_t *vctx, pcilib_event_id_t event_id, void *data) {
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
