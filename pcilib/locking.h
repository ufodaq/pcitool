/**
 * @file lock_global.h
 * @brief this file is the header file for functions that touch all locks allocated for software registers.
 * @details for more details about implementation choice, please read the file lock.h
 */
#define _XOPEN_SOURCE 700

#ifndef _LOCKING_
#define _LOCKING_

#include <pthread.h>

/** number of maximum locks*/
#define PCILIB_MAX_NUMBER_LOCKS 64

/**size of one lock, determine so the size of the protocol_name in the way locks are registered. 40 bytes are necessary for the mutex structure, so we have a protocol name of length LOCK_SIZE-40*/
#define PCILIB_LOCK_SIZE 128

/** number of locks per page of kernel memory*/
#define PCILIB_LOCKS_PER_PAGE PCILIB_KMEM_PAGE_SIZE/PCILIB_LOCK_SIZE

/** number of pages allocated for locks in kernel memory*/
#define PCILIB_NUMBER_OF_LOCK_PAGES (PCILIB_MAX_NUMBER_LOCKS*PCILIB_LOCK_SIZE)/PCILIB_KMEM_PAGE_SIZE


/**
* new type to define a semaphore. It was made to differentiate from the library type.
*/
typedef pthread_mutex_t pcilib_lock_t;

/**
 * this function destroy all locks created 
 *@param[in] ctx, the pcilib_t running
 */
void pcilib_clean_all_locks(pcilib_t* ctx);

/**
* this function initialize the kmem pages containing locks
*@param[in] ctx the pcilib_t running
*/
int pcilib_init_locking(pcilib_t* ctx, ...);

/**
 * this function destroys all locks and then free the kernel memory allocated for them before
 * @param[in] ctx the pcilib_t running
 */
void pcilib_free_all_locks(pcilib_t* ctx);

#endif /* _LOCK_GLOBAL_ */
