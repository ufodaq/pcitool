#define _IPECAMERA_IMAGE_C
#define _BSD_SOURCE
#define _GNU_SOURCE

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
#include "../event.h"

#include "pcilib.h"
#include "private.h"
#include "model.h"
#include "reader.h"
#include "events.h"
#include "data.h"

#include "dma/nwl_dma.h"

#ifdef IPECAMERA_DEBUG
#include "dma/nwl.h"
#endif /* IPECAMERA_DEBUG */


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

	FIND_REG(status3_reg, "fpga", "status3");

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
//    ipecamera_t *ctx = (ipecamera_t*)vctx;
    
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


int ipecamera_start(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_event_flags_t flags) {
    int i;
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
    ctx->preproc_id = 0;
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
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate ring buffer (%lu bytes)", ctx->padded_size * ctx->buffer_size);
	return PCILIB_ERROR_MEMORY;
    }

    ctx->image = (ipecamera_pixel_t*)malloc(ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
    if (!ctx->image) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate image buffer (%lu bytes)", ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
	return PCILIB_ERROR_MEMORY;
    }

    ctx->cmask = malloc(ctx->dim.height * ctx->buffer_size * sizeof(ipecamera_change_mask_t));
    if (!ctx->cmask) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate change-mask buffer");
	return PCILIB_ERROR_MEMORY;
    }

    ctx->frame = (ipecamera_frame_t*)malloc(ctx->buffer_size * sizeof(ipecamera_frame_t));
    if (!ctx->frame) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate frame-info buffer");
	return PCILIB_ERROR_MEMORY;
    }
    
    memset(ctx->frame, 0, ctx->buffer_size * sizeof(ipecamera_frame_t));
    
    for (i = 0; i < ctx->buffer_size; i++) {
	err = pthread_rwlock_init(&ctx->frame[i].mutex, NULL);
	if (err) break;
    }

    ctx->frame_mutex_destroy = i;

    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Initialization of rwlock mutexes for frame synchronization has failed");
	return PCILIB_ERROR_FAILED;
    }
    
    ctx->ipedec = ufo_decoder_new(ctx->dim.height, ctx->dim.width, NULL, 0);
    if (!ctx->ipedec) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
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
*/

    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	return err;
    }

	// Clean DMA
    err = pcilib_skip_dma(vctx->pcilib, ctx->rdma);
    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Can't start grabbing, device continuously writes unexpected data using DMA engine");
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
    
    if (flags&PCILIB_EVENT_FLAG_PREPROCESS) {
	ctx->n_preproc = pcilib_get_cpu_count();
	
	    // it would be greate to detect hyperthreading cores and ban them
	switch (ctx->n_preproc) {
	    case 1: break;
	    case 2-3: ctx->n_preproc -= 1; break;
	    default: ctx->n_preproc -= 2; break;
	}

	if ((vctx->params.parallel.max_threads)&&(vctx->params.parallel.max_threads < ctx->n_preproc))
	    ctx->n_preproc = vctx->params.parallel.max_threads;

	ctx->preproc = (ipecamera_preprocessor_t*)malloc(ctx->n_preproc * sizeof(ipecamera_preprocessor_t));
	if (!ctx->preproc) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Unable to allocate memory for preprocessor contexts");
	    return PCILIB_ERROR_MEMORY;
	}

	memset(ctx->preproc, 0, ctx->n_preproc * sizeof(ipecamera_preprocessor_t));

	err = pthread_mutex_init(&ctx->preproc_mutex, NULL);
	if (err) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Failed to initialize event mutex");
	    return PCILIB_ERROR_FAILED;
	}
	ctx->preproc_mutex_destroy = 1;
	

	ctx->run_preprocessors = 1;
	for (i = 0; i < ctx->n_preproc; i++) {
	    ctx->preproc[i].i = i;
	    ctx->preproc[i].ipecamera = ctx;
	    err = pthread_create(&ctx->preproc[i].thread, NULL, ipecamera_preproc_thread, ctx->preproc + i);
	    if (err) {
		err = PCILIB_ERROR_FAILED;
		break;
	    } else {
		ctx->preproc[i].started = 1;
	    }
	}
	
	if (err) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Failed to schedule some of the preprocessor threads");
	    return err;
	}
    } else {
	ctx->n_preproc = 0;
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
	err = PCILIB_ERROR_FAILED;
    }
    
    pthread_attr_destroy(&attr);    

    return err;
}


int ipecamera_stop(pcilib_context_t *vctx, pcilib_event_flags_t flags) {
    int i;
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
    
    if (ctx->preproc) {
	ctx->run_preprocessors = 0;
	
	for (i = 0; i < ctx->n_preproc; i++) {
	    if (ctx->preproc[i].started) {
		pthread_join(ctx->preproc[i].thread, &retcode);
		ctx->preproc[i].started = 0;
	    }
	}

	if (ctx->preproc_mutex_destroy) {
	    pthread_mutex_destroy(&ctx->preproc_mutex);
	    ctx->preproc_mutex_destroy = 0;
	}
	
	free(ctx->preproc);
	ctx->preproc = NULL;
    }
    
    if (ctx->frame_mutex_destroy) {
	for (i = 0; i < ctx->frame_mutex_destroy; i++) {
	    pthread_rwlock_destroy(&ctx->frame[i].mutex);
	}
	ctx->frame_mutex_destroy = 0;
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

    if (ctx->frame) {
	free(ctx->frame);
	ctx->frame = NULL;
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
    
    pcilib_sleep_until_deadline(&ctx->next_trigger);

/*
    do {
	usleep(10);
	GET_REG(status3_reg, value);
    } while (value&0x20000000);
*/
    
    SET_REG(control_reg, IPECAMERA_FRAME_REQUEST|IPECAMERA_READOUT_FLAG);
    usleep(IPECAMERA_WAIT_FRAME_RCVD_TIME);
    CHECK_REG(status_reg, IPECAMERA_EXPECTED_STATUS);
    SET_REG(control_reg, IPECAMERA_IDLE|IPECAMERA_READOUT_FLAG);


    pcilib_calc_deadline(&ctx->next_trigger, IPECAMERA_NEXT_FRAME_DELAY);

    return 0;
}
