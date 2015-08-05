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

int pcilib_clean_kernel_memory(pcilib_t *ctx, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags) {
    kmem_handle_t kh = {0};
    kh.use = use;
    kh.flags = flags|PCILIB_KMEM_FLAG_MASS;

    return ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_FREE, &kh);
}


static int pcilib_free_kernel_buffer(pcilib_t *ctx, pcilib_kmem_list_t *kbuf, size_t i, pcilib_kmem_flags_t flags) {
    kmem_handle_t kh = {0};

    if (kbuf->buf.blocks[i].ua) munmap(kbuf->buf.blocks[i].ua, kbuf->buf.blocks[i].size + kbuf->buf.blocks[i].alignment_offset);
    kh.handle_id = kbuf->buf.blocks[i].handle_id;
    kh.pa = kbuf->buf.blocks[i].pa;
    kh.flags = flags;
    
    return ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_FREE, &kh);
}

static void pcilib_cancel_kernel_memory(pcilib_t *ctx, pcilib_kmem_list_t *kbuf, pcilib_kmem_flags_t flags, int last_flags) {
    int ret;
    
    if (!kbuf->buf.n_blocks) return;

	// consistency error during processing of last block, special treatment could be needed
    if (last_flags) {
	pcilib_kmem_flags_t failed_flags = flags;
	
	if (last_flags&KMEM_FLAG_REUSED_PERSISTENT) flags&=~PCILIB_KMEM_FLAG_PERSISTENT;
	if (last_flags&KMEM_FLAG_REUSED_HW) flags&=~PCILIB_KMEM_FLAG_HARDWARE;
	
	if (failed_flags != flags) {
	    ret = pcilib_free_kernel_buffer(ctx, kbuf, --kbuf->buf.n_blocks, failed_flags);
	    if (ret) pcilib_error("PCIDRIVER_IOC_KMEM_FREE ioctl have failed");
	}
    }

    pcilib_free_kernel_memory(ctx, kbuf, flags);
}

pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags) {
    int err = 0;
    const char *error = NULL;
    
    int ret;
    int i;
    void *addr;
    
    pcilib_tristate_t reused = PCILIB_TRISTATE_NO;
    int persistent = -1;
    int hardware = -1;

    kmem_handle_t kh = {0};
    
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)malloc(sizeof(pcilib_kmem_list_t) + nmemb * sizeof(pcilib_kmem_addr_t));
    if (!kbuf) {
	pcilib_error("Memory allocation has failed");
	return NULL;
    }
    
    memset(kbuf, 0, sizeof(pcilib_kmem_list_t) + nmemb * sizeof(pcilib_kmem_addr_t));

    err = pcilib_lock(ctx->locks.mmap);
    if (err) {
	pcilib_error("Error (%i) acquiring mmap lock", err);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_KMEM );
    if (ret) {
	pcilib_unlock(ctx->locks.mmap);
	pcilib_error("PCIDRIVER_IOC_MMAP_MODE ioctl have failed");
	return NULL;
    }
    
    kh.type = type;
    kh.size = size;
    kh.align = alignment;
    kh.use = use;

    if ((type&PCILIB_KMEM_TYPE_MASK) == PCILIB_KMEM_TYPE_REGION) {
	kh.align = 0;
    } else if ((type&PCILIB_KMEM_TYPE_MASK) != PCILIB_KMEM_TYPE_PAGE) {
	kh.size += alignment;
    }

    for ( i = 0; i < nmemb; i++) {
	kh.item = i;
	kh.flags = flags;

	if ((type&PCILIB_KMEM_TYPE_MASK) == PCILIB_KMEM_TYPE_REGION) {
	    kh.pa = alignment + i * size;
	}
	
        ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh);
	if (ret) {
	    kbuf->buf.n_blocks = i;
	    error = "PCIDRIVER_IOC_KMEM_ALLOC ioctl have failed";
	    break;
	}
	
	kbuf->buf.blocks[i].handle_id = kh.handle_id;
	kbuf->buf.blocks[i].pa = kh.pa;
	kbuf->buf.blocks[i].size = kh.size;
	
	if (!i) reused = (kh.flags&KMEM_FLAG_REUSED)?PCILIB_TRISTATE_YES:PCILIB_TRISTATE_NO;

        if (kh.flags&KMEM_FLAG_REUSED) {
	    if (!i) reused = PCILIB_TRISTATE_YES;
	    else if (!reused) reused = PCILIB_TRISTATE_PARTIAL;
	
	    if (persistent) {
		if (persistent < 0) {
		    /*if (((flags&PCILIB_KMEM_FLAG_PERSISTENT) == 0)&&(kh.flags&KMEM_FLAG_REUSED_PERSISTENT)) err = PCILIB_ERROR_INVALID_STATE;
		    else*/ persistent = (kh.flags&KMEM_FLAG_REUSED_PERSISTENT)?1:0;
		} else if ((kh.flags&KMEM_FLAG_REUSED_PERSISTENT) == 0) err = PCILIB_ERROR_INVALID_STATE;
	    } else if (kh.flags&KMEM_FLAG_REUSED_PERSISTENT) err = PCILIB_ERROR_INVALID_STATE;
	    
	    if (hardware) {
		if (hardware < 0) {
		    /*if (((flags&PCILIB_KMEM_FLAG_HARDWARE) == 0)&&(kh.flags&KMEM_FLAG_REUSED_HW)) err = PCILIB_ERROR_INVALID_STATE;
		    else*/ hardware = (kh.flags&KMEM_FLAG_REUSED_HW)?1:0;
		} else if ((kh.flags&KMEM_FLAG_REUSED_HW) == 0) err = PCILIB_ERROR_INVALID_STATE;
	    } else if (kh.flags&KMEM_FLAG_REUSED_HW) err = PCILIB_ERROR_INVALID_STATE;
	    
	} else {
	    if (!i) reused = PCILIB_TRISTATE_NO;
	    else if (reused) reused = PCILIB_TRISTATE_PARTIAL;
	    
	    if ((persistent > 0)&&((flags&PCILIB_KMEM_FLAG_PERSISTENT) == 0)) err = PCILIB_ERROR_INVALID_STATE;
	    if ((hardware > 0)&&((flags&PCILIB_KMEM_FLAG_HARDWARE) == 0)) err = PCILIB_ERROR_INVALID_STATE;
	}
	
	if (err) {
	    kbuf->buf.n_blocks = i + 1;
	    break;
	}
    
        if ((kh.align)&&((kh.type&PCILIB_KMEM_TYPE_MASK) != PCILIB_KMEM_TYPE_PAGE)) {
	    if (kh.pa % kh.align) kbuf->buf.blocks[i].alignment_offset = kh.align - kh.pa % kh.align;
	    kbuf->buf.blocks[i].size -= kh.align;
	}

    	addr = mmap( 0, kbuf->buf.blocks[i].size + kbuf->buf.blocks[i].alignment_offset, PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
	if ((!addr)||(addr == MAP_FAILED)) {
	    kbuf->buf.n_blocks = i + 1;
	    error = "Failed to mmap allocated kernel memory";
	    break;
	}

	kbuf->buf.blocks[i].ua = addr;
//	if (use == PCILIB_KMEM_USE_DMA_PAGES) {
//	memset(addr, 10, kbuf->buf.blocks[i].size + kbuf->buf.blocks[i].alignment_offset);
//	}
	
	kbuf->buf.blocks[i].mmap_offset = kh.pa & ctx->page_mask;
    }

    pcilib_unlock(ctx->locks.mmap);


	//This is possible in the case of error (nothing is allocated yet) or if buffers are not reused
    if (persistent < 0) persistent = 0;
    if (hardware < 0) hardware = 0;

    if (err||error) {
	pcilib_kmem_flags_t free_flags = 0;
	
	    // for the sake of simplicity always clean partialy reused buffers
	if ((persistent == PCILIB_TRISTATE_PARTIAL)||((persistent <= 0)&&(flags&PCILIB_KMEM_FLAG_PERSISTENT))) {
	    free_flags |= PCILIB_KMEM_FLAG_PERSISTENT;
	}
	
	if ((hardware <= 0)&&(flags&PCILIB_KMEM_FLAG_HARDWARE)) {
	    free_flags |= PCILIB_KMEM_FLAG_HARDWARE;
	}
	
	    // do not clean if we have reused peresistent buffers
	    //  we don't care about -1, because it will be the value only if no buffers actually allocated
	if ((!persistent)||(reused != PCILIB_TRISTATE_YES)) {
	    pcilib_cancel_kernel_memory(ctx, kbuf, free_flags, err?kh.flags:0);
	}

	if (!error) error = "Reused buffers are inconsistent";
	pcilib_error(error);

	return NULL;
    }
    
    if (nmemb == 1) {
	memcpy(&kbuf->buf.addr, &kbuf->buf.blocks[0], sizeof(pcilib_kmem_addr_t));
    }
    
    kbuf->buf.reused = reused|(persistent?PCILIB_KMEM_REUSE_PERSISTENT:0)|(hardware?PCILIB_KMEM_REUSE_HARDWARE:0);
    kbuf->buf.n_blocks = nmemb;
    
    kbuf->prev = NULL;
    kbuf->next = ctx->kmem_list;
    if (ctx->kmem_list) ctx->kmem_list->prev = kbuf;
    ctx->kmem_list = kbuf;

    return (pcilib_kmem_handle_t*)kbuf;
}

void pcilib_free_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_flags_t flags) {
    int ret, err = 0; 
    int i;
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;

	// if linked in to the list
    if (kbuf->next) kbuf->next->prev = kbuf->prev;
    if (kbuf->prev) kbuf->prev->next = kbuf->next;
    else if (ctx->kmem_list == kbuf) ctx->kmem_list = kbuf->next;

    for (i = 0; i < kbuf->buf.n_blocks; i++) {
        ret = pcilib_free_kernel_buffer(ctx, kbuf, i, flags);
    	if ((ret)&&(!err)) err = ret;
    }
    
    free(kbuf);
    
    if (err) {
	pcilib_error("PCIDRIVER_IOC_KMEM_FREE ioctl have failed");
    }
}

/*
int pcilib_kmem_sync(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir) {
    int i;
    int ret;
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    
    for (i = 0; i < kbuf->buf.n_blocks; i++) {
	ret = pcilib_kmem_sync_block(ctx, k, dir, i);
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_KMEM_SYNC ioctl have failed");
	    return PCILIB_ERROR_FAILED;
	}
    }
    
    return 0;    
}
*/

int pcilib_kmem_sync_block(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir, size_t block) {
    int ret;
    kmem_sync_t ks;
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;

    ks.dir = dir;
    ks.handle.handle_id = kbuf->buf.blocks[block].handle_id;
    ks.handle.pa = kbuf->buf.blocks[block].pa;
    ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_SYNC, &ks);
    if (ret) {
	pcilib_error("PCIDRIVER_IOC_KMEM_SYNC ioctl have failed");
	return PCILIB_ERROR_FAILED;
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

uintptr_t pcilib_kmem_get_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
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

uintptr_t pcilib_kmem_get_block_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].pa + kbuf->buf.blocks[block].alignment_offset;
}

size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].size;
}

pcilib_kmem_reuse_state_t  pcilib_kmem_is_reused(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.reused;
}
