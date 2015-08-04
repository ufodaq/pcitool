/**
 * @file lock.h
 * @skip author zilio nicolas, nicolas.zilio@hotmail.fr
 * @brief this file is the header file for the functions that implement a semaphore API for the pcitool program, using pthread robust mutexes.
 * @details the use of pthread robust mutexes  was chosen due to the fact we privilege security over fastness, and that pthread mutexes permits to recover semaphores even with crash ,and that it does not require access to resources that can be easily accessible from extern usage as flock file locking mechanism. A possible other locking mechanism could be the sysv semaphores, but we have a problem of how determine a perfect hash for the init function, and more, benchmarks proves that sysv semaphore aren't that stable and that with more than 10 locks/unlocks, pthread is better in performance, so that should suits more to the final pcitool program. 
 * We considered that mutex implmentation is enough compared to a reader/writer implementation. If it should change, please go to sysv semaphore.
 * Basic explanation on how semaphores here work: a semaphore here is a positive integer, thus that can't go below zero, which is initiated with a value. when a process want access to the critical resource, it asks to decrement the value of the semaphore, and when it has finished, it reincrements it.basically, when the semaphore is equal to zero, any process must have to wait for it to be reincremented before decrementing it again. Here are defined two types of access to the semaphore corresponding to the reader/writer problem : an exclusive lock, which means that no other process than the one who have the resource can access it; a shared lock, which means that other processes who want to access to the resource with a shared lock can have the access, but a concurrent process who want to access the semaphore with an exclusive lock won't be able to.
 * explanation on locks here : here locks are registered in kernel memory, where they are defined by a pthread_mutex_t and a name, which corresponds to a register or processus. The iterations like searching a lock are done on names.
 */

#ifndef _PCILIB_LOCK_H
#define _PCILIB_LOCK_H

#define PCILIB_LOCK_SIZE 128		/**< size of one lock, determine so the size of the protocol_name in the way locks are registered. 40 bytes are necessary for the mutex structure, so we have a protocol name of length LOCK_SIZE-40*/

#include <pcilib.h>

/**
 * type that defines possible flags when locking a lock by calling pcilib_lock
 */
typedef enum {
    PCILIB_LOCK_FLAGS_DEFAULT = 0,	/**< Default flags */
    PCILIB_LOCK_FLAG_UNLOCKED = 1,	/**< Perform operation unlocked (protected by global flock during initialization of locking subsystem) */
    PCILIB_LOCK_FLAG_PERSISTENT = 2	/**< Do not create robust mutexes, but preserve the lock across application launches */
} pcilib_lock_flags_t;

typedef struct pcilib_lock_s pcilib_lock_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * this function initialize a new semaphore in the kernel given a name that corresponds to a specific processus if the semaphore is not already initialized given the name that permits to differentiate semaphores, and then return the integer that points to the semaphore that have been initialized or to a previously already initialized semaphore.
 * @param[in] lock - pointer to lock to initialize
 * @param[in] flags - flags
 * @param[in] lock_id - lock identificator
 * @return error code or 0 on success
 */
int pcilib_init_lock(pcilib_lock_t *lock, pcilib_lock_flags_t flags, const char *lock_id);

/**
 * this function uninitialize a lock in kernel memory and set the corresponding name to 0
 * @param[in] lock_ctx the pointer that points to the lock.
 */
void pcilib_free_lock(pcilib_lock_t *lock_ctx);


const char *pcilib_lock_get_name(pcilib_lock_t *lock);


/**
 * Increment reference count. Not thread/process safe unless system supports stdatomic (gcc 4.9+).
 * In this case, the access should be synchronized by the caller
 * @param[in] lock - pointer to initialized lock
 */
void pcilib_lock_ref(pcilib_lock_t *lock);

/**
 * Decrement reference count. Not thread/process safe unless system supports stdatomic (gcc 4.9+).
 * In this case, the access should be synchronized by the caller
 * @param[in] lock - pointer to initialized lock
 */
void pcilib_lock_unref(pcilib_lock_t *lock);

/**
 * Return _approximate_ number of lock references. The crashed applications will may not unref.
 * @param[in] lock - pointer to initialized lock
 */
size_t pcilib_lock_get_refs(pcilib_lock_t *lock);

pcilib_lock_flags_t pcilib_lock_get_flags(pcilib_lock_t *lock);

/**
 * this function will take a lock for the mutex pointed by lock
 * @param[in] lock the pointer to the mutex
 * @param[in] flags define the type of lock wanted 
 * @param[in] timeout defines timeout 
 */
int pcilib_lock_custom(pcilib_lock_t* lock, pcilib_lock_flags_t flags, pcilib_timeout_t timeout);

/**
 * this function will take a lock for the mutex pointed by lock
 * @param[in] lock the pointer to the mutex
 */
int pcilib_lock(pcilib_lock_t* lock);

/**
  * this function will unlock the lock pointed by lock 
 * @param[in] lock the integer that points to the semaphore
 */
void pcilib_unlock(pcilib_lock_t* lock);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_LOCK_H */
