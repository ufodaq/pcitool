#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "pcilib.h"
#include "pci.h"
#include "kmem.h"
#include "error.h"

pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags) {
    int ret;
    int i;
    void *addr;

    kmem_handle_t kh = {0};
    
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)malloc(sizeof(pcilib_kmem_list_t) + nmemb * sizeof(pcilib_kmem_addr_t));
    if (!kbuf) {
	pcilib_error("Memory allocation has failed");
	return NULL;
    }
    
    memset(kbuf, 0, sizeof(pcilib_kmem_list_t) + nmemb * sizeof(pcilib_kmem_addr_t));
    

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_KMEM );
    if (ret) {
	pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed");
	return NULL;
    }
    
    kh.type = type;
    kh.size = size;
    kh.align = alignment;
    kh.use = use;

    if (type != PCILIB_KMEM_TYPE_PAGE) {
	kh.size += alignment;
    }
    
    for ( i = 0; i < nmemb; i++) {
        ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh);
	if (ret) {
	    kbuf->buf.n_blocks = i;
	    pcilib_free_kernel_memory(ctx, kbuf);
	    pcilib_error("PCIDRIVER_IOC_KMEM_ALLOC ioctl have failed");
	    return NULL;
	}
    
	kbuf->buf.blocks[i].handle_id = kh.handle_id;
	kbuf->buf.blocks[i].pa = kh.pa;
	kbuf->buf.blocks[i].size = kh.size;
    
        if ((alignment)&&(type != PCILIB_KMEM_TYPE_PAGE)) {
	    if (kh.pa % alignment) kbuf->buf.blocks[i].alignment_offset = alignment - kh.pa % alignment;
	    kbuf->buf.blocks[i].size -= alignment;
	}
	
    	addr = mmap( 0, kh.size + kbuf->buf.blocks[i].alignment_offset, PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
	if ((!addr)||(addr == MAP_FAILED)) {
	    kbuf->buf.n_blocks = i + 1;
	    pcilib_free_kernel_memory(ctx, kbuf);
	    pcilib_error("Failed to mmap allocated kernel memory");
	    return NULL;
	}

	kbuf->buf.blocks[i].ua = addr;
	
	kbuf->buf.blocks[i].mmap_offset = kh.pa & ctx->page_mask;
    }
    
    if (nmemb == 1) {
	memcpy(&kbuf->buf.addr, &kbuf->buf.blocks[0], sizeof(pcilib_kmem_addr_t));
    }
    
    kbuf->buf.n_blocks = nmemb;
    
    kbuf->prev = NULL;
    kbuf->next = ctx->kmem_list;
    if (ctx->kmem_list) ctx->kmem_list->prev = kbuf;
    ctx->kmem_list = kbuf;

    return (pcilib_kmem_handle_t*)kbuf;
}

void pcilib_free_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    int ret, err = 0; 
    int i;
    kmem_handle_t kh = {0};
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;

	// if linked in to the list
    if (kbuf->next) kbuf->next->prev = kbuf->prev;
    if (kbuf->prev) kbuf->prev->next = kbuf->next;
    else if (ctx->kmem_list == kbuf) ctx->kmem_list = kbuf->next;

    for (i = 0; i < kbuf->buf.n_blocks; i++) {
        if (kbuf->buf.blocks[i].ua) munmap(kbuf->buf.blocks[i].ua, kbuf->buf.blocks[i].size + kbuf->buf.blocks[i].alignment_offset);

        kh.handle_id = kbuf->buf.blocks[i].handle_id;
        kh.pa = kbuf->buf.blocks[i].pa;
	ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_FREE, &kh);
	if ((ret)&&(!err)) err = ret;
    }
    
    free(kbuf);
    
    if (err) {
	pcilib_error("PCIDRIVER_IOC_KMEM_FREE ioctl have failed");
    }
}

int pcilib_sync_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir) {
    int i;
    int ret;
    kmem_sync_t ks;
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    
    ks.dir = dir;
    
    for (i = 0; i < kbuf->buf.n_blocks; i++) {
        ks.handle.handle_id = kbuf->buf.blocks[i].handle_id;
	ks.handle.pa = kbuf->buf.blocks[i].pa;
	ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_SYNC, &ks);
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_KMEM_SYNC ioctl have failed");
	    return PCILIB_ERROR_FAILED;
	}
	
	if (!kbuf->buf.blocks[i].pa) {
	    kbuf->buf.blocks[i].pa = ks.handle.pa;
	}
    }
    
    return 0;    
}


void *pcilib_kmem_get_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.addr.ua + kbuf->buf.addr.alignment_offset + kbuf->buf.addr.mmap_offset;
}

uintptr_t pcilib_kmem_get_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.addr.pa + kbuf->buf.addr.alignment_offset;
}

void *pcilib_kmem_get_block_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].ua + kbuf->buf.blocks[block].alignment_offset + kbuf->buf.blocks[block].mmap_offset;
}

uintptr_t pcilib_kmem_get_block_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].pa + kbuf->buf.blocks[block].alignment_offset;
}

size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].size;
}
