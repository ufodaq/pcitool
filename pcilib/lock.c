#define _GNU_SOURCE
#define _XOPEN_SOURCE 600

#include <string.h>
#include <stdint.h>
#include "error.h"
#include "lock.h"
#include "pci.h"
#include <stdio.h>
/*
 * this function will take the lock for the semaphore pointed by semId
 */
void pcilib_lock(pcilib_lock_t *lock_ctx, pcilib_lock_flags_t flags, ...){
	int err;
	struct timespec *time;
	va_list pa;
	va_start(pa,flags);

	if(flags & MUTEX_LOCK){
	err=pthread_mutex_lock(lock_ctx);/**< we try to lock here*/
	if(errno!=EOWNERDEAD && err!=0) pcilib_error("can't acquire lock %s, errno %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)),errno);
	/** if the lock haven't been acquired and errno==EOWNERDEAD, it means the previous application that got the lock crashed, we have to remake the lock "consistent" so*/
	else if(errno==EOWNERDEAD){
		pthread_mutex_consistent(lock_ctx);
		pthread_mutex_lock(lock_ctx);
		if(err!=0) pcilib_error("can't acquire lock %s, errno %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)),errno);
	}
	}
	else if(flags & MUTEX_TRYLOCK){
	err=pthread_mutex_trylock(lock_ctx);/**< we try to lock here*/
	if(errno!=EOWNERDEAD && err!=0) pcilib_error("can't acquire lock %s, errno %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)),errno);
	else if(errno==EOWNERDEAD){
		pthread_mutex_consistent(lock_ctx);
		pthread_mutex_lock(lock_ctx);
		if(err!=0) pcilib_error("can't acquire lock %s, errno %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)),errno);
	}
	}
	else if(flags & MUTEX_TIMEDLOCK){
	  time=va_arg(pa,struct timespec*);
	  va_end(pa);
	  err=pthread_mutex_timedlock(lock_ctx, time);/**< we try to lock here*/
	  if(errno!=EOWNERDEAD && err!=0) pcilib_error("can't acquire lock %s, errni %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)),errno);
	else if(errno==EOWNERDEAD){
		pthread_mutex_consistent(lock_ctx);
		pthread_mutex_timedlock(lock_ctx, time);
		if(err!=0) pcilib_error("can't acquire lock %s, errno %i",(char*)(lock_ctx+sizeof(pcilib_lock_t)), errno);
	}
	}
	else pcilib_error("wrong flag for pcilib_lock");
}

/**
 * this function will unlock the semaphore pointed by lock_ctx. 
 */
void pcilib_unlock(pcilib_lock_t* lock_ctx){
	int err;
	if((err=pthread_mutex_unlock(lock_ctx))!=0) pcilib_error("can't unlock mutex: errno %i",errno);
}

/**
 * pcilib_init_lock 
 * this function initialize a new semaphore in the kernel if it's not already initialized given the key that permits to differentiate semaphores, and then return the integer that points to the semaphore that have been initialized or to a previously already initialized semaphore
 * @param[out] lock_ctx the pointer that will points to the semaphore for other functions
 * @param[in] keysem the integer that permits to define to what the semaphore is attached
 */
pcilib_lock_t* pcilib_init_lock(pcilib_t *ctx, char* lock_id, ...){
	int err;
	pthread_mutexattr_t attr;
	int i,j;
	void* addr,*locks_addr;
	va_list pa;
	va_start(pa,lock_id);
	
	char* temp;
	temp=malloc((strlen(lock_id)+strlen("bank_register_"))*sizeof(char));
	sprintf(temp,"bank_register_%s",lock_id);
	
	pcilib_lock_init_flags_t flag;
	flag=va_arg(pa,pcilib_lock_init_flags_t);
	va_end(pa);

	if(strlen(temp)>PCILIB_LOCK_SIZE-sizeof(pcilib_lock_t)) pcilib_error("the entered protocol name is too long");
	if(((PCILIB_MAX_NUMBER_LOCKS*PCILIB_LOCK_SIZE)%PCILIB_KMEM_PAGE_SIZE)!=0) pcilib_error("PCILIB_MAX_NUMBER_LOCKS*PCILIB_LOCK_SIZE should be a multiple of kmem page size");
	
	addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,0);
	if((flag & LOCK_INIT)==0) pcilib_lock((pcilib_lock_t*)addr,MUTEX_LOCK);
	/* we search for the given lock if it was already initialized*/
	for(j=0;j<PCILIB_NUMBER_OF_LOCK_PAGES;j++){
		i=0;
		locks_addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,j);
			while((*(char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t))!=0) && (i<PCILIB_LOCKS_PER_PAGE)){
		  if(strcmp(temp,(char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t)))==0){
			    return (pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE);}
		 i++;
		}
		if(i<PCILIB_LOCKS_PER_PAGE) break;
	}
	if(i==PCILIB_LOCKS_PER_PAGE) pcilib_error("no more free space for a new lock\n");
	/* if not, we create a new one*/
	if((err= pthread_mutexattr_init(&attr))!=0) pcilib_error("can't initialize mutex attribute, errno %i",errno);
		if((err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))!=0) pcilib_error("can't set a shared mutex, errno %i", errno);
		if((err= pthread_mutexattr_setrobust(&attr,PTHREAD_MUTEX_ROBUST))!=0) pcilib_error("can't set a robust mutex, errno: %i",errno);
		if((err=pthread_mutex_init((pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE),&attr))!=0) pcilib_error("can't set attributes to mutex, errno : %i",errno);
		pthread_mutexattr_destroy(&attr);

		strcpy((char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t)),temp);

		if((flag & LOCK_INIT)==0) pcilib_unlock((pcilib_lock_t*)addr);
		return (pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE);
	
}

/*
 * pcilib_lock_free does nothing for now, as we don't want to erase possible locks in the kernel memory (or at any erasing, we should rearrange the locks to have no free space between locks). A thing i think of would be to use chained lists in kernel memory, but how?
 */
void pcilib_free_lock(pcilib_lock_t *lock_ctx){
   int err;
   if((err=pthread_mutex_destroy(lock_ctx))==-1) pcilib_warning("can't clean lock properly, errno %i",errno);
   memset((char*)(lock_ctx+sizeof(pcilib_lock_t)),0,PCILIB_LOCK_SIZE-sizeof(pcilib_lock_t));
 }
