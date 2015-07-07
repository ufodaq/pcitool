#define _XOPEN_SOURCE 700

#include "error.h"
#include "pci.h"
#include <sys/file.h>
#include <string.h>
#include "lock.h"

/*
 * this function clean all locks created by the pcitool program
 */
void pcilib_clean_all_locks(pcilib_t* ctx){
int i,j;
void* addr;
	i=0;
	while(i<PCILIB_NUMBER_OF_LOCK_PAGES){
addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,i);
for(j=0;j<PCILIB_LOCKS_PER_PAGE;j++)
	  pcilib_free_lock((pcilib_lock_t*)(addr+i*PCILIB_LOCK_SIZE));
		i++;
	}

}

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
	
	
	if((err=flock(ctx->handle,LOCK_EX))==-1) pcilib_warning("can't get flock on /dev/fpga0");
	handle=pcilib_alloc_kernel_memory(ctx,PCILIB_KMEM_TYPE_PAGE,PCILIB_NUMBER_OF_LOCK_PAGES,PCILIB_KMEM_PAGE_SIZE,0,PCILIB_KMEM_USE(PCILIB_KMEM_USE_MUTEXES,0),PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);

	if (!handle) {
         pcilib_error("Allocation of kernel memory for mutexes has failed");
         return 1;
    }

	ctx->locks_handle=handle;
        reused = pcilib_kmem_is_reused(ctx, handle);
	#define DEBUG_REUSE
#ifdef DEBUG_REUSE
reused=0;
#endif
	if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
	    pcilib_register_t i;

	    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
		pcilib_error("Inconsistent memory for locks was found (only part of required buffers is available)");
	        pcilib_clean_all_locks(ctx);
		return 1;
	    }
            for(i=0;i<PCILIB_NUMBER_OF_LOCK_PAGES;i++){
                memset(pcilib_kmem_get_block_ua(ctx,handle,i),0,PCILIB_KMEM_PAGE_SIZE);
            }
            pcilib_init_lock(ctx,"pcilib_locking",LOCK_INIT);
	
	}
	    
	if((err=flock(ctx->handle,LOCK_UN))==-1) pcilib_warning("could not remove lock correctly on /dev/fpga0");

	return 0;
}
