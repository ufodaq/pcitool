/**
 * @file lock.h
 * @skip author zilio nicolas, nicolas.zilio@hotmail.fr
 * @brief this file is the header file for the functions that implement a semaphore API for the pcitool program, using pthread robust mutexes.
 * @details the use of pthread robust mutexes  was chosen due to the fact we privilege security over fastness, and that pthread mutexes permits to recover semaphores even with crash ,and that it does not require access to resources that can be easily accessible from extern usage as flock file locking mechanism. A possible other locking mechanism could be the sysv semaphores, but we have a problem of how determine a perfect hash for the init function, and more, benchmarks proves that sysv semaphore aren't that stable and that with more than 10 locks/unlocks, pthread is better in performance, so that should suits more to the final pcitool program. 
 * We considered that mutex implmentation is enough compared to a reader/writer implementation. If it should change, please go to sysv semaphore.
 *  Basic explanation on how semaphores here work: a semaphore here is a positive integer, thus that can't go below zero, which is initiated with a value. when a process want access to the critical resource, it asks to decrement the value of the semaphore, and when it has finished, it reincrements it.basically, when the semaphore is equal to zero, any process must have to wait for it to be reincremented before decrementing it again. Here are defined two types of access to the semaphore corresponding to the reader/writer problem : an exclusive lock, which means that no other process than the one who have the resource can access it; a shared lock, which means that other processes who want to access to the resource with a shared lock can have the access, but a concurrent process who want to access the semaphore with an exclusive lock won't be able to.
 * explanation on locks here : here locks are registered in kernel memory, where they are defined by a pthread_mutex_t and a name, which corresponds to a register or processus. The iterations like searching a lock are done on names.
 */

#ifndef _LOCK_
#define _LOCK_

#include "lock_global.h"

/**
 * type that defines possible flags when locking a lock by calling pcilib_lock
 */
typedef enum{
  MUTEX_LOCK=1,
  MUTEX_TRYLOCK=2,
  MUTEX_TIMEDLOCK=4
} pcilib_lock_flags_t;

/**
 * type that defines flags for the pcilib_init function
 */
typedef enum{
  PCILIB_LOCK=0,
  PCILIB_NO_LOCK=16
}pcilib_lock_init_flags_t;



/**
  * this function initialize a new semaphore in the kernel given a name that corresponds to a specific processus if the semaphore is not already initialized given the name that permits to differentiate semaphores, and then return the integer that points to the semaphore that have been initialized or to a previously already initialized semaphore.
 * @param[in] ctx the pcilib_t running
 * @param [in] flag the flag that let(s us to know if we have to lock during the creation of a new lock
 * @param[in] lock_id the format string that will contains the supplement args 
 *@return a mutex for the given protocol name
 */
pcilib_lock_t* pcilib_init_lock(pcilib_t *ctx, pcilib_lock_init_flags_t flag,char* lock_id, ...);

/**
 * this function uninitialize a lock in kernel memory and set the corresponding name to 0
 * @param[in] lock_ctx the pointer that points to the lock.
 */
void pcilib_free_lock(pcilib_lock_t *lock_ctx);

/**
 * this function will take a lock for the mutex pointed by lock_ctx
 * @param[in] lock_ctx the pointer to the mutex
 * @param[in] lock_flags define the type of lock wanted : MUTEX_LOCK (normal lock), MUTEX_TRYLOCK (will return if it can't acquire lock), MUTEX_TIMEDLOCK (will wait if it can't acquire lock for a given time, after it returns) and in this last case, we need to provide a timespec structure as supplementary argument
 * 
 */
void pcilib_lock(pcilib_lock_t* lock_ctx,pcilib_lock_flags_t flags, ...);

/**
  * this function will unlock the lock pointed by lock_ctx 
 * @param[in] lock_ctx the integer that points to the semaphore
 */
void pcilib_unlock(pcilib_lock_t* lock_ctx);

#endif /*_LOCK_*/
