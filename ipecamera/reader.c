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

#include "pcilib.h"
#include "private.h"
#include "reader.h"

static inline int ipecamera_new_frame(ipecamera_t *ctx) {
    ctx->frame[ctx->buffer_pos].event.raw_size = ctx->cur_size;

    if (ctx->cur_size < ctx->raw_size) ctx->frame[ctx->buffer_pos].event.info.flags |= PCILIB_EVENT_INFO_FLAG_BROKEN;
    
    ctx->buffer_pos = (++ctx->event_id) % ctx->buffer_size;
    ctx->cur_size = 0;

    ctx->frame[ctx->buffer_pos].event.info.type = PCILIB_EVENT0;
    ctx->frame[ctx->buffer_pos].event.info.flags = 0;
    ctx->frame[ctx->buffer_pos].event.image_ready = 0;

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
    int res;
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
	    ctx->frame[ctx->buffer_pos].event.info.seqnum = ((uint32_t*)buf)[6] & 0xF0000000;
	    ctx->frame[ctx->buffer_pos].event.info.offset = ((uint32_t*)buf)[7] & 0xF0000000;
*/
	    ctx->frame[ctx->buffer_pos].event.info.seqnum = ctx->event_id + 1;

	    gettimeofday(&ctx->frame[ctx->buffer_pos].event.info.timestamp, NULL);
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
	res = ctx->event.params.rawdata.callback(ctx->event_id, (pcilib_event_info_t*)(ctx->frame + ctx->buffer_pos), (eof?PCILIB_EVENT_FLAG_EOF:PCILIB_EVENT_FLAGS_DEFAULT), bufsize, buf, ctx->event.params.rawdata.user);
	if (res <= 0) {
	    if (res < 0) return res;
	    ctx->run_reader = 0;
	}
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

void *ipecamera_reader_thread(void *user) {
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
	} //else printf("no error\n");

	//usleep(1000);
    }
    
    ctx->run_streamer = 0;
    
    if (ctx->cur_size)
	pcilib_error("partialy read frame after stop signal, %zu bytes in the buffer", ctx->cur_size);

    return NULL;
}
