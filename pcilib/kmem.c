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
	
	if (last_flags&KMEM_FLAG_REUSED_PERSISTENT) failed_flags&=~PCILIB_KMEM_FLAG_PERSISTENT;
	if (last_flags&KMEM_FLAG_REUSED_HW) failed_flags&=~PCILIB_KMEM_FLAG_HARDWARE;
	
	if (failed_flags != flags) {
	    ret = pcilib_free_kernel_buffer(ctx, kbuf, --kbuf->buf.n_blocks, failed_flags);
	    if (ret) pcilib_error("PCIDRIVER_IOC_KMEM_FREE ioctl have failed");
	}
    }

    pcilib_free_kernel_memory(ctx, kbuf, flags);
}

pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags) {
    int err = 0;
    char error[256];
    
    int ret;
    size_t i, allocated = nmemb;
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

    err = pcilib_lock_global(ctx);
    if (err) {
	pcilib_error("Error (%i) acquiring mmap lock", err);
	return NULL;
    }

    ret = ioctl( ctx->handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_KMEM );
    if (ret) {
	pcilib_unlock_global(ctx);
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

    for ( i = 0; (i < nmemb)||(flags&PCILIB_KMEM_FLAG_MASS); i++) {
	kh.item = i;
	kh.flags = flags;

	if (i >= nmemb) 
	    kh.flags |= KMEM_FLAG_TRY;

	if ((type&PCILIB_KMEM_TYPE_MASK) == PCILIB_KMEM_TYPE_REGION) {
	    kh.pa = alignment + i * size;
	}
	
        ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh);
	if (ret) {
	    kbuf->buf.n_blocks = i;
	    if ((i < nmemb)||(errno != ENOENT)) {
		err = PCILIB_ERROR_FAILED;
		if (errno == EINVAL) {
		    if (kh.type != type)
			sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  we have requested type %u but buffer is of type %lu", use, i, type, kh.type);
		    else if (kh.size != size)
			sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  we have requested size %lu but buffer is of size %lu", use, i, size, kh.size);
		    else if (kh.align != alignment)
			sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  we have requested alignment %lu but buffer is of alignment %lu", use, i, size, kh.size);
		    else if ((kh.flags&KMEM_FLAG_EXCLUSIVE) != (flags&KMEM_FLAG_EXCLUSIVE))
			sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  we have requested size %s but buffer is of size %s", use, i, ((flags&KMEM_FLAG_EXCLUSIVE)?"exclusive":"non-exclusive"), ((kh.flags&KMEM_FLAG_EXCLUSIVE)?"exclusive":"non exclusive"));
		    else 
			sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  unknown consistency error", use, i);
		} else if (errno == EBUSY) {
		    sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  reuse counter of kmem_entry is overflown", use, i);
		} else if (errno == ENOMEM) {
		    sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  memory allocation (%zu bytes) failed", use, i, size);
		} else {
		    sprintf(error, "Driver prevents us from re-using buffer (use 0x%x, block: %zu),  PCIDRIVER_IOC_KMEM_ALLOC ioctl have failed with errno %i", use, i, errno);
		}
	    }
	    break;
	}
	
	if (i >= allocated) {
	    void *kbuf_new = realloc(kbuf, sizeof(pcilib_kmem_list_t) + 2 * allocated * sizeof(pcilib_kmem_addr_t));
	    if (!kbuf_new) {
		kbuf->buf.n_blocks = i;
		err = PCILIB_ERROR_MEMORY;
		sprintf(error, "Failed to allocate extra %zu bytes of user-space memory for kmem structures", allocated * sizeof(pcilib_kmem_addr_t));
		break;
	    }
	    memset(kbuf_new + sizeof(pcilib_kmem_list_t) + allocated * sizeof(pcilib_kmem_addr_t) , 0, allocated * sizeof(pcilib_kmem_addr_t));
	    kbuf = kbuf_new;
	    allocated *= 2;
	}
	
	kbuf->buf.blocks[i].handle_id = kh.handle_id;
	kbuf->buf.blocks[i].pa = kh.pa;
	kbuf->buf.blocks[i].ba = kh.ba;
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
	    if (err) {
		kbuf->buf.n_blocks = i + 1;
		sprintf(error,  "Mistmatch in persistent modes of the re-used kmem blocks. Current buffer (use 0x%x, block: %zu) is %s, but prior ones %s", 
		    use, i, ((kh.flags&KMEM_FLAG_REUSED_PERSISTENT)?"persistent":"not persistent"), (persistent?"are":"are not"));
		break;
	    }

	    if (hardware) {
		if (hardware < 0) {
		    /*if (((flags&PCILIB_KMEM_FLAG_HARDWARE) == 0)&&(kh.flags&KMEM_FLAG_REUSED_HW)) err = PCILIB_ERROR_INVALID_STATE;
		    else*/ hardware = (kh.flags&KMEM_FLAG_REUSED_HW)?1:0;
		} else if ((kh.flags&KMEM_FLAG_REUSED_HW) == 0) err = PCILIB_ERROR_INVALID_STATE;
	    } else if (kh.flags&KMEM_FLAG_REUSED_HW) err = PCILIB_ERROR_INVALID_STATE;
	    if (err) {
		kbuf->buf.n_blocks = i + 1;
		sprintf(error,  "Mistmatch in hardware modes of the re-used kmem blocks. Current buffer (use 0x%x, block: %zu) is %s, but prior ones %s", 
		    use, i, ((kh.flags&KMEM_FLAG_REUSED_HW)?"hardware-locked":"not hardware-locked"), (hardware?"are":"are not"));
		break;
	    }
	} else {
	    if (!i) reused = PCILIB_TRISTATE_NO;
	    else if (reused) reused = PCILIB_TRISTATE_PARTIAL;

	    if ((persistent > 0)&&((flags&PCILIB_KMEM_FLAG_PERSISTENT) == 0)) {
		err = PCILIB_ERROR_INVALID_STATE;
		sprintf(error, "Expecting to re-use persistent blocks, but buffer (use 0x%x, block: %zu) is not", use, i);
	    }
	    else if ((hardware > 0)&&((flags&PCILIB_KMEM_FLAG_HARDWARE) == 0)) {
		err = PCILIB_ERROR_INVALID_STATE;
		sprintf(error, "Expecting to re-use hardware-locked blocks, but buffer (use 0x%x, block: %zu) is not", use, i);
	    }
	    if (err) {
		kbuf->buf.n_blocks = i + 1;
		break;
	    }
	}
	

        if ((kh.align)&&((kh.type&PCILIB_KMEM_TYPE_MASK) != PCILIB_KMEM_TYPE_PAGE)) {
    		//  Physical or bus address here?
    	    if (kh.ba) {
	        if (kh.ba % kh.align) kbuf->buf.blocks[i].alignment_offset = kh.align - kh.ba % kh.align;
    	    } else {
	        if (kh.pa % kh.align) kbuf->buf.blocks[i].alignment_offset = kh.align - kh.pa % kh.align;
	    }
	    kbuf->buf.blocks[i].size -= kh.align;
	}

    	addr = mmap( 0, kbuf->buf.blocks[i].size + kbuf->buf.blocks[i].alignment_offset, PROT_WRITE | PROT_READ, MAP_SHARED, ctx->handle, 0 );
	if ((!addr)||(addr == MAP_FAILED)) {
	    kbuf->buf.n_blocks = i + 1;
	    err = PCILIB_ERROR_FAILED;
	    sprintf(error, "Driver prevents us from mmaping buffer (use 0x%x, block: %zu), mmap have failed with errno %i", use, i, errno);
	    break;
	}

	kbuf->buf.blocks[i].ua = addr;
	kbuf->buf.blocks[i].mmap_offset = kh.pa & ctx->page_mask;
    }

    if (err) kbuf->buf.n_blocks = i + 1;
    else kbuf->buf.n_blocks = i;


	// Check if there are more unpicked buffers
    if ((!err)&&((flags&PCILIB_KMEM_FLAG_MASS) == 0)&&(reused == PCILIB_TRISTATE_YES)&&((type&PCILIB_KMEM_TYPE_MASK) != PCILIB_KMEM_TYPE_REGION)) {
	kh.item = kbuf->buf.n_blocks;
	kh.flags = KMEM_FLAG_REUSE|KMEM_FLAG_TRY;

        ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_ALLOC, &kh);
	if (!ret) {
	    kh.flags = KMEM_FLAG_REUSE;
	    ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_FREE, &kh);
	    reused = PCILIB_TRISTATE_PARTIAL;
	} else if (errno != ENOENT) {
	    reused = PCILIB_TRISTATE_PARTIAL;
	}
    }

    pcilib_unlock_global(ctx);


	//This is possible in the case of error (nothing is allocated yet) or if buffers are not reused
    if (persistent < 0) persistent = 0;
    if (hardware < 0) hardware = 0;

    if (err) {
	    // do not clean if we have reused (even partially) persistent/hardware-locked buffers
	if (((persistent)||(hardware))&&(reused != PCILIB_TRISTATE_NO)) {
	    pcilib_cancel_kernel_memory(ctx, kbuf, KMEM_FLAG_REUSE, 0);
	} else {
	    pcilib_kmem_flags_t free_flags = 0;
	    if (flags&PCILIB_KMEM_FLAG_PERSISTENT) {
		free_flags |= PCILIB_KMEM_FLAG_PERSISTENT;
	    }
	    if (flags&PCILIB_KMEM_FLAG_HARDWARE) {
		free_flags |= PCILIB_KMEM_FLAG_HARDWARE;
	    }
		// err indicates consistensy error. The last ioctl have succeeded and we need to clean it in a special way
	    pcilib_cancel_kernel_memory(ctx, kbuf, free_flags, (err == PCILIB_ERROR_INVALID_STATE)?kh.flags:0);
	}

	pcilib_warning("Error %i: %s", err, error);
	return NULL;
    }
    
    if (nmemb == 1) {
	memcpy(&kbuf->buf.addr, &kbuf->buf.blocks[0], sizeof(pcilib_kmem_addr_t));
    }

    kbuf->buf.type = type;
    kbuf->buf.use = use;
    kbuf->buf.reused = reused|(persistent?PCILIB_KMEM_REUSE_PERSISTENT:0)|(hardware?PCILIB_KMEM_REUSE_HARDWARE:0);

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

    switch (kbuf->buf.type) {
      case PCILIB_KMEM_TYPE_DMA_S2C_PAGE:
      case PCILIB_KMEM_TYPE_DMA_C2S_PAGE:
      case PCILIB_KMEM_TYPE_REGION_S2C:
      case PCILIB_KMEM_TYPE_REGION_C2S:
        ks.dir = dir;
	ks.handle.handle_id = kbuf->buf.blocks[block].handle_id;
	ks.handle.pa = kbuf->buf.blocks[block].pa;
	
	ret = ioctl(ctx->handle, PCIDRIVER_IOC_KMEM_SYNC, &ks);
	if (ret) {
	    pcilib_error("PCIDRIVER_IOC_KMEM_SYNC ioctl have failed");
	    return PCILIB_ERROR_FAILED;
	}
	break;
      default:
	;
    }

    return 0;
}

void* volatile pcilib_kmem_get_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.addr.ua + kbuf->buf.addr.alignment_offset + kbuf->buf.addr.mmap_offset;
}

uintptr_t pcilib_kmem_get_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.addr.pa + kbuf->buf.addr.alignment_offset;
}

uintptr_t pcilib_kmem_get_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;

    if (kbuf->buf.addr.ba)
	return kbuf->buf.addr.ba + kbuf->buf.addr.alignment_offset;

    return 0;
}

void* volatile pcilib_kmem_get_block_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].ua + kbuf->buf.blocks[block].alignment_offset + kbuf->buf.blocks[block].mmap_offset;
}

uintptr_t pcilib_kmem_get_block_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].pa + kbuf->buf.blocks[block].alignment_offset;
}

uintptr_t pcilib_kmem_get_block_ba(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;

    if (kbuf->buf.blocks[block].ba)
	return kbuf->buf.blocks[block].ba + kbuf->buf.blocks[block].alignment_offset;

    return 0;
}

size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.blocks[block].size;
}

pcilib_kmem_reuse_state_t  pcilib_kmem_is_reused(pcilib_t *ctx, pcilib_kmem_handle_t *k) {
    pcilib_kmem_list_t *kbuf = (pcilib_kmem_list_t*)k;
    return kbuf->buf.reused;
}
