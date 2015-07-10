#define _XOPEN_SOURCE 700

#include "error.h"
#include "pci.h"
#include <sys/file.h>
#include <string.h>
#include "lock.h"

/*
 * this function destroys all locks without touching at kernel memory directly
 */
void pcilib_clean_all_locks(pcilib_t* ctx){
int i,j;
void* addr;
	i=0;
	/* iteration on all locks*/
	while(i<PCILIB_NUMBER_OF_LOCK_PAGES){
addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,i);
for(j=0;j<PCILIB_LOCKS_PER_PAGE;j++)
	  pcilib_free_lock((pcilib_lock_t*)(addr+i*PCILIB_LOCK_SIZE));
		i++;
	}

}

/*
 * this functions destroy all locks and then free the kernel memory allocated for them
 */
void pcilib_free_locking(pcilib_t* ctx){
  pcilib_clean_all_locks(ctx);
  pcilib_free_kernel_memory(ctx,ctx->locks_handle,PCILIB_KMEM_FLAG_REUSE);
}

/*
 * this function allocates the kernel memory for the locks for software registers
 */
int pcilib_init_locking(pcilib_t* ctx, ...){
/*for future possible more args
	va_list pa;
	va_start(pa,ctx);*/
	pcilib_kmem_handle_t *handle;
	int err;
	pcilib_kmem_reuse_state_t reused;
	
	/* we flock() to make sure to not have two initialization in the same time (possible long time to init)*/
	if((err=flock(ctx->handle,LOCK_EX))==-1) pcilib_warning("can't get flock on /dev/fpga0");

	handle=pcilib_alloc_kernel_memory(ctx,PCILIB_KMEM_TYPE_PAGE,PCILIB_NUMBER_OF_LOCK_PAGES,PCILIB_KMEM_PAGE_SIZE,0,PCILIB_KMEM_USE(PCILIB_KMEM_USE_MUTEXES,0),PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);

	if (!handle) {
         pcilib_error("Allocation of kernel memory for mutexes has failed");
         return 1;
    }

	ctx->locks_handle=handle;
        reused = pcilib_kmem_is_reused(ctx, handle);
//#define DEBUG_REUSE
#ifdef DEBUG_REUSE
reused=0;
#endif
/* verification about the handling memory got, first use or not, and integrity*/
	if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
	    pcilib_register_t i;

	    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
		pcilib_error("Inconsistent memory for locks was found (only part of required buffers is available)");
	        pcilib_clean_all_locks(ctx);
		return 1;
	    }
	    /* if we get here so this is the first initialization (after some free or new), we so set kernel pages to 0 and initialize then the first lock that will be used when we create other locks*/
            for(i=0;i<PCILIB_NUMBER_OF_LOCK_PAGES;i++){
                memset(pcilib_kmem_get_block_ua(ctx,handle,i),0,PCILIB_KMEM_PAGE_SIZE);
            }
            pcilib_init_lock(ctx,PCILIB_NO_LOCK,"%s","pcilib_first_locking");
	
	}
	    
	if((err=flock(ctx->handle,LOCK_UN))==-1) pcilib_warning("could not remove lock correctly on /dev/fpga0");

	return 0;
}
