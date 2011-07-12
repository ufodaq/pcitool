#define NWL_RING_GET(data, offset)  *(uint32_t*)(((char*)(data)) + (offset))
#define NWL_RING_SET(data, offset, val)  *(uint32_t*)(((char*)(data)) + (offset)) = (val)
#define NWL_RING_UPDATE(data, offset, mask, val) *(uint32_t*)(((char*)(data)) + (offset)) = ((*(uint32_t*)(((char*)(data)) + (offset)))&(mask))|(val)

int dma_nwl_allocate_engine_buffers(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info) {
    int err = 0;

    int i;
    uint32_t val;
    uint32_t buf_sz;
    uint64_t buf_pa;

    char *base = info->base_addr;
    
    if (info->pages) return 0;
    
    pcilib_kmem_handle_t *ring = pcilib_alloc_kernel_memory(ctx->pcilib, PCILIB_KMEM_TYPE_CONSISTENT, 1, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE, PCILIB_NWL_ALIGNMENT, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA, info->desc.addr), 0);
    pcilib_kmem_handle_t *pages = pcilib_alloc_kernel_memory(ctx->pcilib, PCILIB_KMEM_TYPE_PAGE, PCILIB_NWL_DMA_PAGES, 0, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA, info->desc.addr), 0);

    if ((ring)&&(pages)) err = dma_nwl_sync_buffers(ctx, info, pages);
    else err = PCILIB_ERROR_FAILED;


    if (err) {
	if (pages) pcilib_free_kernel_memory(ctx->pcilib, pages);
	if (ring) pcilib_free_kernel_memory(ctx->pcilib, ring);    
	return err;
    }
    
    unsigned char *data = (unsigned char*)pcilib_kmem_get_ua(ctx->pcilib, ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, ring);
    
    memset(data, 0, PCILIB_NWL_DMA_PAGES * PCILIB_NWL_DMA_DESCRIPTOR_SIZE);

    for (i = 0; i < PCILIB_NWL_DMA_PAGES; i++, data += PCILIB_NWL_DMA_DESCRIPTOR_SIZE) {
	buf_pa = pcilib_kmem_get_block_pa(ctx->pcilib, pages, i);
	buf_sz = pcilib_kmem_get_block_size(ctx->pcilib, pages, i);

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
    
    info->ring = ring;
    info->pages = pages;
    info->page_size = buf_sz;
    info->ring_size = PCILIB_NWL_DMA_PAGES;
    
    info->head = 0;
    info->tail = 0;
    
    return 0;
}


static size_t dma_nwl_clean_buffers(nwl_dma_t * ctx, pcilib_nwl_engine_description_t *info) {
    size_t res = 0;
    uint32_t status, control;

    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

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
	info->tail++;
	if (info->tail == info->ring_size) {
	    ring -= (info->tail - 1) * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	    info->tail = 0;
	} else {
	    ring += PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
	}
	
	res++;

	if (info->tail != info->head) goto next_buffer;
    }
    
//    printf("====> Cleaned: %i\n", res);
    return res;
}


static size_t dma_nwl_get_next_buffer(nwl_dma_t * ctx, pcilib_nwl_engine_description_t *info, size_t n_buffers, pcilib_timeout_t timeout) {
    struct timeval start, cur;

    size_t res, n = 0;
    size_t head;

    for (head = info->head; (((head + 1)%info->ring_size) != info->tail)&&(n < n_buffers); head++, n++);
    if (n == n_buffers) return info->head;

    gettimeofday(&start, NULL);

    res = dma_nwl_clean_buffers(ctx, info);
    if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
    else n += res;

    
    while (n < n_buffers) {
	if (timeout != PCILIB_TIMEOUT_INFINITE) {
	    gettimeofday(&cur, NULL);
	    if  (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > timeout) break;
	}
	
	usleep (10);	

        res = dma_nwl_clean_buffers(ctx, info);
        if (res == (size_t)-1) return PCILIB_DMA_BUFFER_INVALID;
	else if (res > 0) {
	    gettimeofday(&start, NULL);
	    n += res;
	}
    }
    
    if (n < n_buffers) return PCILIB_DMA_BUFFER_INVALID;
    
    return info->head;
}

static int dma_nwl_push_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, size_t size, int eop, pcilib_timeout_t timeout) {
    int flags;
    
    uint32_t val;
    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);

    ring += info->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    
    if (!info->writting) {
	flags |= DMA_BD_SOP_MASK;
	info->writting = 1;
    }
    if (eop) {
	flags |= DMA_BD_EOP_MASK;
	info->writting = 0;
    }

    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, size|flags);
    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, size);

    info->head++;
    if (info->head == info->ring_size) info->head = 0;
    
    val = ring_pa + info->head * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, info->base_addr, REG_SW_NEXT_BD);
//    nwl_read_register(val, ctx, info->base_addr, 0x18);

//    usleep(10000);

//    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_LAST_BD);
//    printf("Last BD(Write): %lx %lx\n", ring, val);
    
    
    return 0;
}


static size_t dma_nwl_wait_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info, size_t *size, int *eop, pcilib_timeout_t timeout) {
    uint32_t val;
    struct timeval start, cur;
    uint32_t status_size, status, control;

//    usleep(10000);
    
    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    
//    status_size = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
//    printf("Status0: %lx\n", status_size);

    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;

    gettimeofday(&start, NULL);
    
//    printf("Waiting %li\n", info->tail);
//    nwl_read_register(val, ctx, info->base_addr, REG_DMA_ENG_LAST_BD);
//    printf("Last BD(Read): %lx %lx\n", ring, val);

    do {
	status_size = NWL_RING_GET(ring, DMA_BD_BUFL_STATUS_OFFSET);
	status = status_size & DMA_BD_STATUS_MASK;
	
//	printf("%i: %lx\n", info->tail, status_size);
    
	if (status & DMA_BD_ERROR_MASK) {
    	    pcilib_error("NWL DMA Engine reported error in ring descriptor");
    	    return (size_t)-1;
	}	
	
	if (status & DMA_BD_COMP_MASK) {
	    if (status & DMA_BD_EOP_MASK) *eop = 1;
	    else *eop = 0;
        
	    *size = status_size & DMA_BD_BUFL_MASK;
	
//	    printf("Status: %lx\n", status_size);
	    return info->tail;
	}
	
	usleep(10);
        gettimeofday(&cur, NULL);
    } while ((timeout == PCILIB_TIMEOUT_INFINITE)||(((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) < timeout));

//    printf("Final status: %lx\n", status_size);
    
    return (size_t)-1;
}

static int dma_nwl_return_buffer(nwl_dma_t *ctx, pcilib_nwl_engine_description_t *info) {
    uint32_t val;

    unsigned char *ring = pcilib_kmem_get_ua(ctx->pcilib, info->ring);
    uint32_t ring_pa = pcilib_kmem_get_pa(ctx->pcilib, info->ring);
    size_t bufsz = pcilib_kmem_get_block_size(ctx->pcilib, info->pages, info->tail);

    ring += info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
//    printf("Returning: %i\n", info->tail);

#ifdef NWL_GENERATE_DMA_IRQ    
    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, bufsz | DMA_BD_INT_ERROR_MASK | DMA_BD_INT_COMP_MASK);
#else /* NWL_GENERATE_DMA_IRQ */
    NWL_RING_SET(ring, DMA_BD_BUFL_CTRL_OFFSET, bufsz);
#endif /* NWL_GENERATE_DMA_IRQ */

    NWL_RING_SET(ring, DMA_BD_BUFL_STATUS_OFFSET, 0);

    val = ring_pa + info->tail * PCILIB_NWL_DMA_DESCRIPTOR_SIZE;
    nwl_write_register(val, ctx, info->base_addr, REG_SW_NEXT_BD);
//    nwl_read_register(val, ctx, info->base_addr, 0x18);
    
    info->tail++;
    if (info->tail == info->ring_size) info->tail = 0;
}
