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
	  /* one question is "is pthread_mutex_consistent protected in case we call twice it?", it seems to not make any importance in fact regarding man pages, but we have to survey it in future applications*/
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
		pthread_mutex_trylock(lock_ctx);
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
 * this function initialize a new semaphore in the kernel if it's not already initialized given the key that permits to differentiate semaphores, and then return the integer that points to the semaphore that have been initialized or to a previously already initialized semaphore
 */
pcilib_lock_t* pcilib_init_lock(pcilib_t *ctx, pcilib_lock_init_flags_t flag, char* lock_id, ...){
	int err;
	pthread_mutexattr_t attr;
	int i,j;
	void* addr,*locks_addr;
	char buffer[PCILIB_LOCK_SIZE-sizeof(pcilib_lock_t)];
	/* here lock_id is the format string for vsprintf, the lock name will so be put in adding arguments*/
	va_list pa;
	va_start(pa,lock_id);
	err=vsprintf(buffer,lock_id,pa);
	va_end(pa);

	if(err<0) pcilib_error("error in obtaining the lock name");
	if(((PCILIB_MAX_NUMBER_LOCKS*PCILIB_LOCK_SIZE)%PCILIB_KMEM_PAGE_SIZE)!=0) pcilib_error("PCILIB_MAX_NUMBER_LOCKS*PCILIB_LOCK_SIZE should be a multiple of kmem page size");
	
	addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,0);
	if((flag & PCILIB_NO_LOCK)==0) pcilib_lock((pcilib_lock_t*)addr,MUTEX_LOCK);
	/* we search for the given lock if it was already initialized*/
	for(j=0;j<PCILIB_NUMBER_OF_LOCK_PAGES;j++){
		i=0;
		locks_addr=pcilib_kmem_get_block_ua(ctx,ctx->locks_handle,j);
			while((*(char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t))!=0) && (i<PCILIB_LOCKS_PER_PAGE)){
		  if(strcmp(buffer,(char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t)))==0){
			    return (pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE);}
		 i++;
		}
		if(i<PCILIB_LOCKS_PER_PAGE) break;
	}
	/* the kernel space could be full*/
	if(i==PCILIB_LOCKS_PER_PAGE) pcilib_error("no more free space for a new lock\n");
	/* if not, we create a new one*/
	if((err= pthread_mutexattr_init(&attr))!=0) pcilib_error("can't initialize mutex attribute, errno %i",errno);
		if((err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))!=0) pcilib_error("can't set a shared mutex, errno %i", errno);
		if((err= pthread_mutexattr_setrobust(&attr,PTHREAD_MUTEX_ROBUST))!=0) pcilib_error("can't set a robust mutex, errno: %i",errno);
		if((err=pthread_mutex_init((pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE),&attr))!=0) pcilib_error("can't set attributes to mutex, errno : %i",errno);
		pthread_mutexattr_destroy(&attr);

		strcpy((char*)(locks_addr+i*PCILIB_LOCK_SIZE+sizeof(pcilib_lock_t)),buffer);

		if((flag & PCILIB_NO_LOCK)==0) pcilib_unlock((pcilib_lock_t*)addr);
		return (pcilib_lock_t*)(locks_addr+i*PCILIB_LOCK_SIZE);
	
}

/*
 * we uninitialize a mutex and set its name to 0 pointed by lock_ctx with this function. setting name to is the real destroying operation, but we need to unitialize the lock to initialize it again after
 */
void pcilib_free_lock(pcilib_lock_t *lock_ctx){
   int err;
   if((err=pthread_mutex_destroy(lock_ctx))==-1) pcilib_warning("can't clean lock properly, errno %i",errno);
   memset((char*)(lock_ctx+sizeof(pcilib_lock_t)),0,PCILIB_LOCK_SIZE-sizeof(pcilib_lock_t));
 }
