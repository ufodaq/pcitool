#define _PCILIB_DMA_IPE_C
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "pci.h"
#include "pcilib.h"
#include "error.h"
#include "tools.h"
#include "debug.h"

#include "ipe.h"
#include "ipe_private.h"




pcilib_dma_context_t *dma_ipe_init(pcilib_t *pcilib, const char *model, const void *arg) {
//    int err = 0;
    
    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);

    ipe_dma_t *ctx = malloc(sizeof(ipe_dma_t));

    if (ctx) {
	memset(ctx, 0, sizeof(ipe_dma_t));
	ctx->dmactx.pcilib = pcilib;

#ifdef IPEDMA_64BIT_MODE
	    // Always supported and we need to use it
	ctx->mode64 = 1;
#endif /* IPEDMA_64BIT_MODE */

	pcilib_register_bank_t dma_bank = pcilib_find_register_bank_by_addr(pcilib, PCILIB_REGISTER_BANK_DMA);

	if (dma_bank == PCILIB_REGISTER_BANK_INVALID) {
	    free(ctx);
	    pcilib_error("DMA Register Bank could not be found");
	    return NULL;
	}

	ctx->dma_bank = model_info->banks + dma_bank;
	ctx->base_addr = pcilib_resolve_register_address(pcilib, ctx->dma_bank->bar, ctx->dma_bank->read_addr);
    }

    return (pcilib_dma_context_t*)ctx;
}

void  dma_ipe_free(pcilib_dma_context_t *vctx) {
    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

    if (ctx) {
	dma_ipe_stop(vctx, PCILIB_DMA_ENGINE_ALL, PCILIB_DMA_FLAGS_DEFAULT);
	free(ctx);
    }
}


int dma_ipe_start(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    size_t i;

    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

#ifndef IPEDMA_TLP_SIZE
    const pcilib_pcie_link_info_t *link_info;
#endif /* ! IPEDMA_TLP_SIZE */

    int preserve = 0;
    pcilib_kmem_flags_t kflags;
    pcilib_kmem_reuse_state_t reuse_desc, reuse_pages;

    volatile void *desc_va;
    volatile uint32_t *last_written_addr_ptr;

    pcilib_register_value_t value;

    int tlp_size;
    uint32_t address64;

    if (dma == PCILIB_DMA_ENGINE_INVALID) return 0;
    else if (dma > 1) return PCILIB_ERROR_INVALID_BANK;

    if (!ctx->started) ctx->started = 1;

    if (flags&PCILIB_DMA_FLAG_PERSISTENT) ctx->preserve = 1;

    if (ctx->pages) return 0;
    
    kflags = PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_EXCLUSIVE|PCILIB_KMEM_FLAG_HARDWARE|(ctx->preserve?PCILIB_KMEM_FLAG_PERSISTENT:0);
    pcilib_kmem_handle_t *desc = pcilib_alloc_kernel_memory(ctx->dmactx.pcilib, PCILIB_KMEM_TYPE_CONSISTENT, 1, IPEDMA_DESCRIPTOR_SIZE, IPEDMA_DESCRIPTOR_ALIGNMENT, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_RING, 0x00), kflags);
    pcilib_kmem_handle_t *pages = pcilib_alloc_kernel_memory(ctx->dmactx.pcilib, PCILIB_KMEM_TYPE_DMA_C2S_PAGE, IPEDMA_DMA_PAGES, 0, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_PAGES, 0x00), kflags);

    if (!desc||!pages) {
	if (pages) pcilib_free_kernel_memory(ctx->dmactx.pcilib, pages, 0);
	if (desc) pcilib_free_kernel_memory(ctx->dmactx.pcilib, desc, 0);
	return PCILIB_ERROR_MEMORY;
    }
    reuse_desc = pcilib_kmem_is_reused(ctx->dmactx.pcilib, desc);
    reuse_pages = pcilib_kmem_is_reused(ctx->dmactx.pcilib, pages);

    if (reuse_desc == reuse_pages) {
	if (reuse_desc & PCILIB_KMEM_REUSE_PARTIAL) pcilib_warning("Inconsistent DMA buffers are found (only part of required buffers is available), reinitializing...");
	else if (reuse_desc & PCILIB_KMEM_REUSE_REUSED) {
	    if ((reuse_desc & PCILIB_KMEM_REUSE_PERSISTENT) == 0) pcilib_warning("Lost DMA buffers are found (non-persistent mode), reinitializing...");
	    else if ((reuse_desc & PCILIB_KMEM_REUSE_HARDWARE) == 0) pcilib_warning("Lost DMA buffers are found (missing HW reference), reinitializing...");
	    else {
#ifndef IPEDMA_BUG_DMARD
# ifndef IPEDMA_STREAMING_MODE
		RD(IPEDMA_REG_PAGE_COUNT, value);

		if (value != IPEDMA_DMA_PAGES) pcilib_warning("Inconsistent DMA buffers are found (Number of allocated buffers (%lu) does not match current request (%lu)), reinitializing...", value + 1, IPEDMA_DMA_PAGES);
		else
# endif /* IPEDMA_STREAMING_MODE */
#endif /* IPEDMA_BUG_DMARD */
		    preserve = 1;
	    }
	}
    } else pcilib_warning("Inconsistent DMA buffers (modes of ring and page buffers does not match), reinitializing....");

    desc_va = pcilib_kmem_get_ua(ctx->dmactx.pcilib, desc);
    if (ctx->mode64) last_written_addr_ptr = desc_va + 3 * sizeof(uint32_t);
    else last_written_addr_ptr = desc_va + 4 * sizeof(uint32_t);

    if (preserve) {
	ctx->reused = 1;
	ctx->preserve = 1;
	
	    // Detect the current state of DMA engine
#ifdef IPEDMA_BUG_DMARD
	FILE *f = fopen("/tmp/pcitool_lastread", "r");
	if (!f) pcilib_error("Can't read current status");
	fread(&value, 1, sizeof(pcilib_register_value_t), f);
	fclose(f);
#else /* IPEDMA_BUG_DMARD */
	RD(IPEDMA_REG_LAST_READ, value);
	    // Numbered from 1 in FPGA
# ifdef IPEDMA_BUG_LAST_READ
	if (value == IPEDMA_DMA_PAGES)
	    value = 0;
# else /* IPEDMA_BUG_LAST_READ */
	value--;
# endif /* IPEDMA_BUG_LAST_READ */
#endif /* IPEDMA_BUG_DMARD */

	ctx->last_read = value;
    } else {
	ctx->reused = 0;

	    // Disable DMA
	WR(IPEDMA_REG_CONTROL, 0x0);
	usleep(IPEDMA_RESET_DELAY);
	
	    // Reset DMA engine
	WR(IPEDMA_REG_RESET, 0x1);
	usleep(IPEDMA_RESET_DELAY);
	WR(IPEDMA_REG_RESET, 0x0);
	usleep(IPEDMA_RESET_DELAY);

#ifndef IPEDMA_BUG_DMARD
	    // Verify PCIe link status
	RD(IPEDMA_REG_RESET, value);
	if ((value != 0x14031700)&&(value != 0x14021700))
	    pcilib_warning("PCIe is not ready, code is %lx", value);
#endif /* IPEDMA_BUG_DMARD */

	    // Enable 64 bit addressing and configure TLP and PACKET sizes (40 bit mode can be used with big pre-allocated buffers later)
	if (ctx->mode64) address64 = 0x8000 | (0<<24);
	else address64 = 0;

#ifdef IPEDMA_TLP_SIZE	
	tlp_size = IPEDMA_TLP_SIZE;
#else /* IPEDMA_TLP_SIZE */
	link_info = pcilib_get_pcie_link_info(vctx->pcilib);
	if (link_info) {
	    tlp_size = 1<<link_info->payload;
	    if (tlp_size > IPEDMA_MAX_TLP_SIZE)
		tlp_size = IPEDMA_MAX_TLP_SIZE;
	} else tlp_size = 128;
#endif /* IPEDMA_TLP_SIZE */
        WR(IPEDMA_REG_TLP_SIZE,  address64 | (tlp_size>>2));
        WR(IPEDMA_REG_TLP_COUNT, IPEDMA_PAGE_SIZE / (tlp_size * IPEDMA_CORES));

	    // Setting progress register threshold
	WR(IPEDMA_REG_UPDATE_THRESHOLD, IPEDMA_DMA_PROGRESS_THRESHOLD);
        
	    // Reseting configured DMA pages
        WR(IPEDMA_REG_PAGE_COUNT, 0);
        
	    // Setting current read position and configuring progress register
#ifdef IPEDMA_BUG_LAST_READ
	WR(IPEDMA_REG_LAST_READ, IPEDMA_DMA_PAGES - 1);
#else /* IPEDMA_BUG_LAST_READ */
	WR(IPEDMA_REG_LAST_READ, IPEDMA_DMA_PAGES);
#endif /* IPEDMA_BUG_LAST_READ */
	WR(IPEDMA_REG_UPDATE_ADDR, pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, desc, 0));

	    // Instructing DMA engine that writting should start from the first DMA page
	*last_written_addr_ptr = 0;
	
	for (i = 0; i < IPEDMA_DMA_PAGES; i++) {
	    uintptr_t bus_addr_check, bus_addr = pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, pages, i);
	    WR(IPEDMA_REG_PAGE_ADDR, bus_addr);
	    if (bus_addr%4096) printf("Bad address %lu: %lx\n", i, bus_addr);

	    RD(IPEDMA_REG_PAGE_ADDR, bus_addr_check);
	    if (bus_addr_check != bus_addr) {
		pcilib_error("Written (%x) and read (%x) bus addresses does not match\n", bus_addr, bus_addr_check);
	    }

	    usleep(IPEDMA_ADD_PAGE_DELAY);
	}

	    // Enable DMA
	WR(IPEDMA_REG_CONTROL, 0x1);
	
	ctx->last_read = IPEDMA_DMA_PAGES - 1;

#ifdef IPEDMA_BUG_DMARD
	FILE *f = fopen("/tmp/pcitool_lastread", "w");
	if (!f) pcilib_error("Can't write current status");
	value = ctx->last_read;
	fwrite(&value, 1, sizeof(pcilib_register_value_t), f);
	fclose(f);
#endif /* IPEDMA_BUG_DMARD */
    }

    ctx->last_read_addr = pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, pages, ctx->last_read);

    ctx->desc = desc;
    ctx->pages = pages;
    ctx->page_size = pcilib_kmem_get_block_size(ctx->dmactx.pcilib, pages, 0);
    ctx->ring_size = IPEDMA_DMA_PAGES;

    return 0;
}

int dma_ipe_stop(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_flags_t flags) {
    pcilib_kmem_flags_t kflags;

    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

    if (!ctx->started) return 0;

    if ((dma != PCILIB_DMA_ENGINE_INVALID)&&(dma > 1)) return PCILIB_ERROR_INVALID_BANK;

	    // ignoring previous setting if flag specified
    if (flags&PCILIB_DMA_FLAG_PERSISTENT) {
	ctx->preserve = 0;
    }

    if (ctx->preserve) {
	kflags = PCILIB_KMEM_FLAG_REUSE;
    } else {
        kflags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT;

	ctx->started  = 0;

	    // Disable DMA
	WR(IPEDMA_REG_CONTROL, 0);
	usleep(IPEDMA_RESET_DELAY);
	
	    // Reset DMA engine
	WR(IPEDMA_REG_RESET, 0x1);
	usleep(IPEDMA_RESET_DELAY);
	WR(IPEDMA_REG_RESET, 0x0);
	usleep(IPEDMA_RESET_DELAY);

	    // Reseting configured DMA pages
        WR(IPEDMA_REG_PAGE_COUNT, 0);
	usleep(IPEDMA_RESET_DELAY);
    }

	// Clean buffers
    if (ctx->desc) {
	pcilib_free_kernel_memory(ctx->dmactx.pcilib, ctx->desc, kflags);
	ctx->desc = NULL;
    }

    if (ctx->pages) {
	pcilib_free_kernel_memory(ctx->dmactx.pcilib, ctx->pages, kflags);
	ctx->pages = NULL;
    }

    return 0;
}

static size_t dma_ipe_find_buffer_by_bus_addr(ipe_dma_t *ctx, uintptr_t bus_addr) {
    size_t i;

    for (i = 0; i < ctx->ring_size; i++) {
	uintptr_t buf_addr = pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, ctx->pages, i);

	if (bus_addr == buf_addr) 
	    return i;
    }

    return (size_t)-1;
}


int dma_ipe_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers) {
    size_t i;
    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

    void *desc_va = (void*)pcilib_kmem_get_ua(ctx->dmactx.pcilib, ctx->desc);
    volatile uint32_t *last_written_addr_ptr;
    uint32_t last_written_addr;

    if (!status) return -1;

    if (ctx->mode64) last_written_addr_ptr = desc_va + 3 * sizeof(uint32_t);
    else last_written_addr_ptr = desc_va + 4 * sizeof(uint32_t);

    pcilib_debug(DMA, "Current DMA status      - last read: %4u, last_read_addr: %4u (0x%x), last_written: %4u (0x%x)", ctx->last_read, 
	dma_ipe_find_buffer_by_bus_addr(ctx, ctx->last_read_addr), ctx->last_read_addr, 
	dma_ipe_find_buffer_by_bus_addr(ctx, *last_written_addr_ptr), *last_written_addr_ptr
    );

    last_written_addr = *last_written_addr_ptr;

    status->started = ctx->started;
    status->ring_size = ctx->ring_size;
    status->buffer_size = ctx->page_size;
    status->written_buffers = 0;
    status->written_bytes = 0;

	// For simplicity, we keep last_read here, and fix in the end
    status->ring_tail = ctx->last_read;

    status->ring_head = dma_ipe_find_buffer_by_bus_addr(ctx, last_written_addr);

    if (status->ring_head == (size_t)-1) {
	if (last_written_addr) {
	    pcilib_warning("DMA is in unknown state, last_written_addr does not correspond any of available buffers");
	    return PCILIB_ERROR_FAILED;
	}
	status->ring_head = 0;
	status->ring_tail = 0;
    }

    if (n_buffers > ctx->ring_size) n_buffers = ctx->ring_size;

    if (buffers)
	memset(buffers, 0, n_buffers * sizeof(pcilib_dma_buffer_status_t));

    if (status->ring_head >= status->ring_tail) {
	for (i = status->ring_tail + 1; i <= status->ring_head; i++) {
	    status->written_buffers++;
	    status->written_bytes += ctx->page_size;

	    if ((buffers)&&(i < n_buffers)) {
		buffers[i].used = 1;
		buffers[i].size = ctx->page_size;
		buffers[i].first = 1;
		buffers[i].last = 1;
	    }
	}
    } else {
	for (i = 0; i <= status->ring_head; i++) {
	    status->written_buffers++;
	    status->written_bytes += ctx->page_size;

	    if ((buffers)&&(i < n_buffers)) {
	        buffers[i].used = 1;
		buffers[i].size = ctx->page_size;
	        buffers[i].first = 1;
	        buffers[i].last = 1;
	    }
	} 
	
	for (i = status->ring_tail + 1; i < status->ring_size; i++) {
	    status->written_buffers++;
	    status->written_bytes += ctx->page_size;

	    if ((buffers)&&(i < n_buffers)) {
		buffers[i].used = 1;
	        buffers[i].size = ctx->page_size;
		buffers[i].first = 1;
	        buffers[i].last = 1;
	    }
	} 
    }

	// We actually keep last_read in the ring_tail, so need to increase
    if (status->ring_tail != status->ring_head) {
	status->ring_tail++;
	if (status->ring_tail == status->ring_size) status->ring_tail = 0;
    }

    return 0;
}


int dma_ipe_stream_read(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, pcilib_timeout_t timeout, pcilib_dma_callback_t cb, void *cbattr) {
    int err, ret = PCILIB_STREAMING_REQ_PACKET;


    pcilib_timeout_t wait = 0;
    struct timeval start, cur;

    volatile void *desc_va;
    volatile uint32_t *last_written_addr_ptr;
    volatile uint32_t *empty_detected_ptr;

    pcilib_dma_flags_t packet_flags = PCILIB_DMA_FLAG_EOP;
    size_t nodata_sleep;

#ifdef IPEDMA_BUG_DMARD
    pcilib_register_value_t value;
#endif /* IPEDMA_BUG_DMARD */

    switch (sched_getscheduler(0)) {
     case SCHED_FIFO:
     case SCHED_RR:
	nodata_sleep = IPEDMA_NODATA_SLEEP;
	break;
     default:
	pcilib_info_once("Streaming DMA data using non real-time thread (may cause extra CPU load)", errno);
	nodata_sleep = 0;
    }

    size_t cur_read;

    ipe_dma_t *ctx = (ipe_dma_t*)vctx;

    err = dma_ipe_start(vctx, dma, PCILIB_DMA_FLAGS_DEFAULT);
    if (err) return err;

    desc_va = (void*)pcilib_kmem_get_ua(ctx->dmactx.pcilib, ctx->desc);

    if (ctx->mode64) last_written_addr_ptr = desc_va + 3 * sizeof(uint32_t);
    else last_written_addr_ptr = desc_va + 4 * sizeof(uint32_t);

    empty_detected_ptr = last_written_addr_ptr - 2;

    do {
	switch (ret&PCILIB_STREAMING_TIMEOUT_MASK) {
	    case PCILIB_STREAMING_CONTINUE: 
		    // Hardware indicates that there is no more data pending and we can safely stop if there is no data in the kernel buffers already
#ifdef IPEDMA_SUPPORT_EMPTY_DETECTED
		if (*empty_detected_ptr)
		    wait = 0;
		else
#endif /* IPEDMA_SUPPORT_EMPTY_DETECTED */
		    wait = IPEDMA_DMA_TIMEOUT; 
	    break;
	    case PCILIB_STREAMING_WAIT: 
		wait = (timeout > IPEDMA_DMA_TIMEOUT)?timeout:IPEDMA_DMA_TIMEOUT;
	    break;
//	    case PCILIB_STREAMING_CHECK: wait = 0; break;
	}

	pcilib_debug(DMA, "Waiting for data in %4u - last_read: %4u, last_read_addr: %4u (0x%08x), last_written: %4u (0x%08x)", ctx->last_read + 1, ctx->last_read, 
		dma_ipe_find_buffer_by_bus_addr(ctx, ctx->last_read_addr), ctx->last_read_addr, 
		dma_ipe_find_buffer_by_bus_addr(ctx, *last_written_addr_ptr), *last_written_addr_ptr
	);

	gettimeofday(&start, NULL);
	memcpy(&cur, &start, sizeof(struct timeval));
	while (((*last_written_addr_ptr == 0)||(ctx->last_read_addr == (*last_written_addr_ptr)))&&((wait == PCILIB_TIMEOUT_INFINITE)||(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < wait))) {
	    if (nodata_sleep) 
		usleep(nodata_sleep);
		
#ifdef IPEDMA_SUPPORT_EMPTY_DETECTED
	    if ((ret != PCILIB_STREAMING_REQ_PACKET)&&(*empty_detected_ptr)) break;
#endif /* IPEDMA_SUPPORT_EMPTY_DETECTED */
	    gettimeofday(&cur, NULL);
	}
	
	    // Failing out if we exited on timeout
	if ((ctx->last_read_addr == (*last_written_addr_ptr))||(*last_written_addr_ptr == 0)) {
#ifdef IPEDMA_SUPPORT_EMPTY_DETECTED
# ifdef PCILIB_DEBUG_DMA
	    if ((wait)&&(*last_written_addr_ptr)&&(!*empty_detected_ptr))
		pcilib_debug(DMA, "The empty_detected flag is not set, but no data arrived within %lu us", wait);
# endif /* PCILIB_DEBUG_DMA */
#endif /* IPEDMA_SUPPORT_EMPTY_DETECTED */
	    return (ret&PCILIB_STREAMING_FAIL)?PCILIB_ERROR_TIMEOUT:0;
	}

	    // Getting next page to read
	cur_read = ctx->last_read + 1;
	if (cur_read == ctx->ring_size) cur_read = 0;

	pcilib_debug(DMA, "Got buffer          %4u - last read: %4u, last_read_addr: %4u (0x%x), last_written: %4u (0x%x)", cur_read, ctx->last_read, 
		dma_ipe_find_buffer_by_bus_addr(ctx, ctx->last_read_addr), ctx->last_read_addr, 
		dma_ipe_find_buffer_by_bus_addr(ctx, *last_written_addr_ptr), *last_written_addr_ptr
	);

#ifdef IPEDMA_DETECT_PACKETS
	if ((*empty_detected_ptr)&&(pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, ctx->pages, cur_read) == (*last_written_addr_ptr))) packet_flags = PCILIB_DMA_FLAG_EOP;
	else packet_flags = 0;
#endif /* IPEDMA_DETECT_PACKETS */
	
	pcilib_kmem_sync_block(ctx->dmactx.pcilib, ctx->pages, PCILIB_KMEM_SYNC_FROMDEVICE, cur_read);
        void *buf = pcilib_kmem_get_block_ua(ctx->dmactx.pcilib, ctx->pages, cur_read);
	ret = cb(cbattr, packet_flags, ctx->page_size, buf);
	if (ret < 0) return -ret;
	
	    // We don't need this because hardware does not intend to read anything from the memory
//	pcilib_kmem_sync_block(ctx->dmactx.pcilib, ctx->pages, PCILIB_KMEM_SYNC_TODEVICE, cur_read);

	    // Return buffer into the DMA pool when processed
#ifdef IPEDMA_STREAMING_MODE
	uintptr_t buf_ba = pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, ctx->pages, cur_read);
	WR(IPEDMA_REG_PAGE_ADDR, buf_ba);    
# ifdef IPEDMA_STREAMING_CHECKS
	pcilib_register_value_t streaming_status;
	RD(IPEDMA_REG_STREAMING_STATUS, streaming_status);
	if (streaming_status)
	    pcilib_error("Invalid status (0x%lx) adding a DMA buffer into the queue", streaming_status);
# endif /* IPEDMA_STREAMING_MODE */
#endif /* IPEDMA_STREAMING_MODE */

	    // Numbered from 1
#ifdef IPEDMA_BUG_LAST_READ
	WR(IPEDMA_REG_LAST_READ, cur_read?cur_read:IPEDMA_DMA_PAGES);
#else /* IPEDMA_BUG_LAST_READ */
	WR(IPEDMA_REG_LAST_READ, cur_read + 1);
#endif /* IPEDMA_BUG_LAST_READ */

	pcilib_debug(DMA, "Buffer returned     %4u - last read: %4u, last_read_addr: %4u (0x%x), last_written: %4u (0x%x)", cur_read, ctx->last_read, 
		dma_ipe_find_buffer_by_bus_addr(ctx, ctx->last_read_addr), ctx->last_read_addr, 
		dma_ipe_find_buffer_by_bus_addr(ctx, *last_written_addr_ptr), *last_written_addr_ptr
	);


	ctx->last_read = cur_read;
	ctx->last_read_addr = pcilib_kmem_get_block_ba(ctx->dmactx.pcilib, ctx->pages, cur_read);

#ifdef IPEDMA_BUG_DMARD
	FILE *f = fopen("/tmp/pcitool_lastread", "w");
	if (!f) pcilib_error("Can't write current status");
	value = cur_read;
	fwrite(&value, 1, sizeof(pcilib_register_value_t), f);
	fclose(f);
#endif /* IPEDMA_BUG_DMARD */

    } while (ret);

    return 0;
}
