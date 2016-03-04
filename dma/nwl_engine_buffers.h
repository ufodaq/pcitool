#define NWL_RING_GET(data, offset)  *(uint32_t*)(((char*)(data)) + (offset))
#define NWL_RING_SET(data, offset, val)  *(uint32_t*)(((char*)(data)) + (offset)) = (val)
#define NWL_RING_UPDATE(data, offset, mask, val) *(uint32_t*)(((char*)(data)) + (offset)) = ((*(uint32_t*)(((char*)(data)) + (offset)))&(mask))|(val)

static int dma_nwl_compute_read_s2c_pointers(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx, unsigned char *ring, uint32_t ring_pa) {
    uint32_t val;

    const char *base = ectx->base_addr;
    
    nwl_read_register(val, ctx, base, REG_SW_NEXT_BD);
    if ((val < ring_pa)||((val - ring_pa) % PCILIB_NWL_DMA_DESCRIPTOR_SIZE)) {
	if (val < ring_pa) pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_SW_NEXT_BD register value (%lx) is below start of ring [%lx,%lx])", val, ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	else pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_SW_NEXT_BD register value (%zu / %u) is fractal)", val - ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	return PCILIB_ERROR_INVALID_STATE;
    }

    ectx->head = (val - ring_pa) / PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    if (ectx->head >= PCILIB_NWL_DMA_PAGES) {
	pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_SW_NEXT_BD register value (%zu) out of range)", ectx->head);
	return PCILIB_ERROR_INVALID_STATE;
    }

    nwl_read_register(val, ctx, base, REG_DMA_ENG_NEXT_BD);
    if ((val < ring_pa)||((val - ring_pa) % PCILIB_NWL_DMA_DESCRIPTOR_SIZE)) {
	if (val < ring_pa) pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_DMA_ENG_NEXT_BD register value (%lx) is below start of ring [%lx,%lx])", val, ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	else pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_DMA_ENG_NEXT_BD register value (%zu / %u) is fractal)", val - ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	return PCILIB_ERROR_INVALID_STATE;
    }

    ectx->tail = (val - ring_pa) / PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    if (ectx->tail >= PCILIB_NWL_DMA_PAGES) {
	pcilib_warning("Inconsistent S2C DMA Ring buffer is found (REG_DMA_ENG_NEXT_BD register value (%zu) out of range)", ectx->tail);
	return PCILIB_ERROR_INVALID_STATE;
    }

    pcilib_debug(DMA, "S2C: %lu %lu\n", ectx->tail, ectx->head);

    return 0;
}

static int dma_nwl_compute_read_c2s_pointers(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx, unsigned char *ring, uint32_t ring_pa) {
    uint32_t val;

    const char *base = ectx->base_addr;

    nwl_read_register(val, ctx, base, REG_SW_NEXT_BD);
    if ((val < ring_pa)||((val - ring_pa) % PCILIB_NWL_DMA_DESCRIPTOR_SIZE)) {
	if (val < ring_pa) pcilib_warning("Inconsistent C2S DMA Ring buffer is found (REG_SW_NEXT_BD register value (%lx) is below start of the ring [%lx,%lx])", val, ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	else pcilib_warning("Inconsistent C2S DMA Ring buffer is found (REG_SW_NEXT_BD register value (%zu / %u) is fractal)", val - ring_pa, PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	return PCILIB_ERROR_INVALID_STATE;
    }

    ectx->head = (val - ring_pa) / PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    if (ectx->head >= PCILIB_NWL_DMA_PAGES) {
	pcilib_warning("Inconsistent C2S DMA Ring buffer is found (REG_SW_NEXT_BD register value (%zu) out of range)", ectx->head);
	return PCILIB_ERROR_INVALID_STATE;
    }
    
    ectx->tail = ectx->head + 1;
    if (ectx->tail == PCILIB_NWL_DMA_PAGES) ectx->tail = 0;

    pcilib_debug(DMA, "C2S: %lu %lu\n", ectx->tail, ectx->head);

    return 0;
}


static int dma_nwl_allocate_engine_buffers(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx) {
    int err = 0;

    int i;
    int preserve = 0;
    uint16_t sub_use;
    uint32_t val;
    uint32_t buf_sz;
    uint64_t buf_pa;
    pcilib_kmem_reuse_state_t reuse_ring, reuse_pages;
    pcilib_kmem_flags_t flags;
    pcilib_kmem_type_t type;

    char *base = ectx->base_addr;
    
    if (ectx->pages) return 0;
    
	// Or bidirectional specified by 0x0|addr, or read 0x0|addr and write 0x80|addr
    type = (ectx->desc->direction == PCILIB_DMA_TO_DEVICE)?PCILIB_KMEM_TYPE_DMA_S2C_PAGE:PCILIB_KMEM_TYPE_DMA_C2S_PAGE;
    sub_use = ectx->desc->addr|((ectx->desc->direction == PCILIB_DMA_TO_DEVICE)?0x80:0x00);
    flags = PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_EXCLUSIVE|PCILIB_KMEM_FLAG_HARDWARE|(ectx->preserve?PCILIB_KMEM_FLAG_PERSISTENT:0);
    
    pcilib_kmem_handle_t *ring = pcilib_alloc_kernel_memory(ctx->dmactx.pcilib, PCILIB_KMEM_TYPE_CONSISTENT, 1, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE, PCILIB_NWL_ALIGNMENT, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_RING, sub_use), flags);
    pcilib_kmem_handle_t *pages = pcilib_alloc_kernel_memory(ctx->dmactx.pcilib, type, PCILIB_NWL_DMA_PAGES, 0, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_PAGES, sub_use), flags);

    if (!ring||!pages) {
	if (pages) pcilib_free_kernel_memory(ctx->dmactx.pcilib, pages, 0);
	if (ring) pcilib_free_kernel_memory(ctx->dmactx.pcilib, ring, 0);
	return PCILIB_ERROR_MEMORY;
    }

    reuse_ring = pcilib_kmem_is_reused(ctx->dmactx.pcilib, ring);
    reuse_pages = pcilib_kmem_is_reused(ctx->dmactx.pcilib, pages);

//	I guess idea here was that we not need to check all that stuff during the second iteration
//	which is basicaly true (shall we expect any driver-triggered changes or parallel accesses?)
//	but still we need to set preserve flag (and that if we enforcing preservation --start-dma). 
//	Probably having checks anyway is not harming...
//    if (!ectx->preserve) {
	if (reuse_ring == reuse_pages) {
	    if (reuse_ring & PCILIB_KMEM_REUSE_PARTIAL) pcilib_warning("Inconsistent DMA buffers are found (only part of required buffers is available), reinitializing...");
	    else if (reuse_ring & PCILIB_KMEM_REUSE_REUSED) {
		if ((reuse_ring & PCILIB_KMEM_REUSE_PERSISTENT) == 0) pcilib_warning("Lost DMA buffers are found (non-persistent mode), reinitializing...");
		else if ((reuse_ring & PCILIB_KMEM_REUSE_HARDWARE) == 0) pcilib_warning("Lost DMA buffers are found (missing HW reference), reinitializing...");
		else {
		    nwl_read_register(val, ctx, ectx->base_addr, REG_DMA_ENG_CTRL_STATUS);

		    if ((val&DMA_ENG_RUNNING) == 0) pcilib_warning("Lost DMA buffers are found (DMA engine is stopped), reinitializing...");
		    else preserve = 1;
		}
	    } 	
	} else pcilib_warning("Inconsistent DMA buffers (modes of ring and page buffers does not match), reinitializing....");
//    }

    
    unsigned char *data = (unsigned char*)pcilib_kmem_get_ua(ctx->dmactx.pcilib, ring);
    uint32_t ring_pa = pcilib_kmem_get_ba(ctx->dmactx.pcilib, ring);

    if (preserve) {
	if (ectx->desc->direction == PCILIB_DMA_FROM_DEVICE) err = dma_nwl_compute_read_c2s_pointers(ctx, ectx, data, ring_pa);
	else err = dma_nwl_compute_read_s2c_pointers(ctx, ectx, data, ring_pa);

	if (err) preserve = 0;
    }
    
    if (preserve) {
	ectx->reused = 1;
        buf_sz = pcilib_kmem_get_block_size(ctx->dmactx.pcilib, pages, 0);
    } else {
	ectx->reused = 0;
	
	memset(data, 0, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE);

	for (i = 0; i < PCILIB_NWL_DMA_PAGES; i++, data += PCILIB_NWL_DMA_DESCRIPTOR_SIZE) {
	    buf_pa = pcilib_kmem_get_block_pa(ctx->dmactx.pcilib, pages, i);
	    buf_sz = pcilib_kmem_get_block_size(ctx->dmactx.pcilib, pages, i);

	    NWL_RING_SET(data, DMA_BD_NDESC_OFFSET, ring_pa + ((i + 1) % PCILIB_NWL_DMA_PAGES) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE);
	    NWL_RING_SET(data, DMA_BD_BUFAL_OFFSET, buf_pa&0xFFFFFFFF);
	    NWL_RING_SET(data, DMA_BD_BUFAH_OFFSET, buf_pa>>32);
#ifdef NWL_GENERATE_DMA_IRQ
    	    NWL_RING_SET(data, DMA_BD_BUFL_CTRL_OFFSET, buf_sz | DMA_BD_INT_ERROR_MASK | DMA_BD_INT_COMP_MASK);
#else /* NWL_GENERATE_DMA_IRQ */
    	    NWL_RING_SET(data, DMA_BD_BUFL_CTRL_OFFSET, buf_sz);
#endif /* NWL_GENERATE_DMA_IRQ */
	}

	val = ring_pa;
	nwl_write_register(val, ctx, base, REG_DMA_ENG_NEXT_BD);
	nwl_write_register(val, ctx, base, REG_SW_NEXT_BD);

        ectx->head = 0;
	ectx->tail = 0;
    }
    
    ectx->ring = ring;
    ectx->pages = pages;
    ectx->page_size = buf_sz;
    ectx->ring_size = PCILIB_NWL_DMA_PAGES;
    
    return 0;
}


static size_t dma_nwl_clean_buffers(nwl_dma_t * ctx, pcilib_nwl_engine_context_t *ectx) {
    size_t res = 0;
    uint32_t status;

    volatile unsigned char *ring = pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);
    ring += ectx->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

next_buffer:
    status = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET)&DMA_BD_STATUS_MASK;
//  control = NWL_RING_GET(ring, DMA_BD_BUFL_CTRL_OFFSET)&DMA_BD_CTRL_MASK;
    
    if (status & DMA_BD_ERROR_MASK) {
        pcilib_error("NWL DMA Engine reported error in ring descriptor");
        return (size_t)-1;
    }
	
    if (status & DMA_BD_SHORT_MASK) {
        pcilib_error("NWL DMA Engine reported short error");
        return (size_t)-1;
    }
	
    if (status & DMA_BD_COMP_MASK) {
	ectx->tail++;
	if (ectx->tail == ectx->ring_size) {
	    ring -= (ectx->tail - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	    ectx->tail = 0;
	} else {
	    ring += PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	}
	
	res++;

	if (ectx->tail != ectx->head) goto next_buffer;
    }
    
//    printf("====> Cleaned: %i\n", res);
    return res;
}


static size_t dma_nwl_get_next_buffer(nwl_dma_t * ctx, pcilib_nwl_engine_context_t *ectx, size_t n_buffers, pcilib_timeout_t timeout) {
    struct timeval start, cur;

    size_t res, n = 0;
    size_t head;

    for (head = ectx->head; (((head + 1)%ectx->ring_size) != ectx->tail)&&(n < n_buffers); head++, n++);
    if (n == n_buffers) return ectx->head;

    gettimeofday(&start, NULL);

    res = dma_nwl_clean_buffers(ctx, ectx);
    if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
    else n += res;

    
    while (n < n_buffers) {
	if (timeout != PCILIB_TIMEOUT_INFINITE) {
	    gettimeofday(&cur, NULL);
	    if  (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > timeout) break;
	}
	
	usleep (10);	

        res = dma_nwl_clean_buffers(ctx, ectx);
        if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
	else if (res > 0) {
	    gettimeofday(&start, NULL);
	    n += res;
	}
    }

    if (n < n_buffers) return PCILIB_DMA_BUFFER_INVALID;
    
    return ectx->head;
}

static int dma_nwl_push_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx, size_t size, int eop, pcilib_timeout_t timeout) {
    int flags = 0;
    
    uint32_t val;
    volatile unsigned char *ring = pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);
    uint32_t ring_pa = pcilib_kmem_get_ba(ctx->dmactx.pcilib, ectx->ring);

    ring += ectx->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    
    if (!ectx->writting) {
	flags |= DMA_BD_SOP_MASK;
	ectx->writting = 1;
    }
    if (eop) {
	flags |= DMA_BD_EOP_MASK;
	ectx->writting = 0;
    }
    
    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, size|flags);
    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, size);

    ectx->head++;
    if (ectx->head == ectx->ring_size) ectx->head = 0;
    
    val = ring_pa + ectx->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, ectx->base_addr, REG_SW_NEXT_BD);
    
    return 0;
}


static size_t dma_nwl_wait_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx, size_t *size, int *eop, pcilib_timeout_t timeout) {
    struct timeval start, cur;
    uint32_t status_size, status;

    volatile unsigned char *ring = pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);
    
    ring += ectx->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    gettimeofday(&start, NULL);
    
    do {
	status_size = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
	status = status_size & DMA_BD_STATUS_MASK;
	
	if (status & DMA_BD_ERROR_MASK) {
    	    pcilib_error("NWL DMA Engine reported error in ring descriptor");
    	    return (size_t)-1;
	}	
	
	if (status & DMA_BD_COMP_MASK) {
	    if (status & DMA_BD_EOP_MASK) *eop = 1;
	    else *eop = 0;
	            
	    *size = status_size & DMA_BD_BUFL_MASK;

/*	    
	    if (mrd) {
		if ((ectx->tail + 1) == ectx->ring_size) ring -= ectx->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
		else ring += PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
		*mrd = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET)&DMA_BD_COMP_MASK;
	    }
*/
	
	    return ectx->tail;
	}
	
	usleep(10);
        gettimeofday(&cur, NULL);
    } while ((timeout == PCILIB_TIMEOUT_INFINITE)||(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < timeout));

    return (size_t)-1;
}

/*
    // This function is not used now, but we may need it in the future
static int dma_nwl_is_overflown(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx) {
    uint32_t status;
    unsigned char *ring = pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);
    if (ectx->tail > 0) ring += (ectx->tail - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    else ring += (ectx->ring_size - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    status = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
    return status&DMA_BD_COMP_MASK?1:0;
}
*/

static int dma_nwl_return_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_context_t *ectx) {
    uint32_t val;

    volatile unsigned char *ring = pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);
    uint32_t ring_pa = pcilib_kmem_get_ba(ctx->dmactx.pcilib, ectx->ring);
    size_t bufsz = pcilib_kmem_get_block_size(ctx->dmactx.pcilib, ectx->pages, ectx->tail);

    ring += ectx->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

#ifdef NWL_GENERATE_DMA_IRQ    
    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, bufsz | DMA_BD_INT_ERROR_MASK | DMA_BD_INT_COMP_MASK);
#else /* NWL_GENERATE_DMA_IRQ */
    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, bufsz);
#endif /* NWL_GENERATE_DMA_IRQ */

    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, 0);

    val = ring_pa + ectx->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, ectx->base_addr, REG_SW_NEXT_BD);
    
    ectx->tail++;
    if (ectx->tail == ectx->ring_size) ectx->tail = 0;
    
    return 0;
}

int dma_nwl_get_status(pcilib_dma_context_t *vctx, pcilib_dma_engine_t dma, pcilib_dma_engine_status_t *status, size_t n_buffers, pcilib_dma_buffer_status_t *buffers) {
    size_t i;
    uint32_t bstatus;
    nwl_dma_t *ctx = (nwl_dma_t*)vctx;
    pcilib_nwl_engine_context_t *ectx = ctx->engines + dma;
    unsigned char *ring = (unsigned char*)pcilib_kmem_get_ua(ctx->dmactx.pcilib, ectx->ring);


    if (!status) return -1;
    
    status->started = ectx->started;
    status->ring_size = ectx->ring_size;
    status->buffer_size = ectx->page_size;
    status->ring_tail = ectx->tail;
    status->written_buffers = 0;
    status->written_bytes = 0;
    
    if (ectx->desc->direction == PCILIB_DMA_FROM_DEVICE) {
	size_t pos = 0;
	for (i = 0; i < ectx->ring_size; i++) {
	    pos = status->ring_tail + i;
	    if (pos >= ectx->ring_size) pos -= ectx->ring_size;

	    bstatus = NWL_RING_GET(ring + pos * PCILIB_NWL_DMA_DESCRIPTOR_SIZE, DMA_BD_BUFL_STATUS_OFFSET);
	    if ((bstatus&(DMA_BD_ERROR_MASK|DMA_BD_COMP_MASK)) == 0) break;
	}
        status->ring_head = pos;
    } else {
        status->ring_head = ectx->head;
    }


    if (buffers) {	
	for (i = 0; (i < ectx->ring_size)&&(i < n_buffers); i++) {
	    bstatus = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);

	    buffers[i].error = bstatus & (DMA_BD_ERROR_MASK/*|DMA_BD_SHORT_MASK*/);
	    buffers[i].used = bstatus & DMA_BD_COMP_MASK;
	    buffers[i].size = bstatus & DMA_BD_BUFL_MASK;
	    buffers[i].first = bstatus & DMA_BD_SOP_MASK;
	    buffers[i].last = bstatus & DMA_BD_EOP_MASK;

	    ring += PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	}
    } 

    for (i = 0; (i < ectx->ring_size)&&(i < n_buffers); i++) {
	bstatus = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
	if (bstatus & DMA_BD_COMP_MASK) {
	    status->written_buffers++;
	    if ((bstatus & (DMA_BD_ERROR_MASK)) == 0)
		status->written_bytes += bstatus & DMA_BD_BUFL_MASK;
	}
    }

    return 0;
}
