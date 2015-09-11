/**
 * @file lock.h
 * @brief this file is the header file for the functions that implement a semaphore API for the pcitool program, using pthread robust mutexes.
 * @details 		the use of pthread robust mutexes  was chosen due to the fact we privilege security over fastness, and that pthread mutexes permits to recover semaphores even with crash ,and that it does not require access to resources that can be easily accessible from extern usage as flock file locking mechanism. A possible other locking mechanism could be the sysv semaphores, but we have a problem of how determine a perfect hash for the init function, and more, benchmarks proves that sysv semaphore aren't that stable. For pure locking/unlocking, pthread is better in performance than sysV, but it suffers from big initialization times. In this sense, a kernel memory space is used for saving the locks, and persistence permits to avoid initializations over uses.
 *
 * 		We considered that mutex implmentation is enough compared to a reader/writer implementation. If it should change, please go to sysv semaphore.
 *
 * 		Basic explanation on how semaphores here work: a semaphore here is a positive integer, thus that can't go below zero, which is initiated with a value. when a process want access to the critical resource, it asks to decrement the value of the semaphore, and when it has finished, it reincrements it.basically, when the semaphore is equal to zero, any process must have to wait for it to be reincremented before decrementing it again. Here are defined two types of access to the semaphore corresponding to the reader/writer problem : an exclusive lock, which means that no other process than the one who have the resource can access it; a shared lock, which means that other processes who want to access to the resource with a shared lock can have the access, but a concurrent process who want to access the semaphore with an exclusive lock won't be able to.
 * explanation on locks here : here locks are registered in kernel memory, where they are defined by a pthread_mutex_t and an identifier name, which corresponds most of the time to a mix of the register associated name and processus (but it's up to the user). The iterations like searching a lock are done on this id name.
 */

#ifndef _PCILIB_LOCK_H
#define _PCILIB_LOCK_H

#define PCILIB_LOCK_SIZE 128		/**< size of one lock. indeed, as we can't allocate easily on the fly memory in the kernel, fixed size have been chosen. determines so the size of the identifier name in the way locks are registered. 40 bytes are necessary for the mutex structure, so we have an id name of length LOCK_SIZE-40*/

#include <pcilib.h>

/**
 * type that defines possible flags for a lock, defining how a lock should be handled by the locking functions
 */
typedef enum {
    PCILIB_LOCK_FLAGS_DEFAULT = 0,	/**< Default flags */
    PCILIB_LOCK_FLAG_UNLOCKED = 1,	/**< Perform operations unlocked, thus without taking care of the lock (protected by global flock during initialization of locking subsystem) */
    PCILIB_LOCK_FLAG_PERSISTENT = 2	/**< Do not create robust mutexes, but preserve the lock across application launches */
} pcilib_lock_flags_t;

/** structure defining a lock*/
typedef struct pcilib_lock_s pcilib_lock_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 *this function initializes a lock, by setting correctly its property given the flags associated.
 * @param[in,out] lock - pointer to lock to initialize
 * @param[in] flags - flags: if it's set to two, then not a robust mutex is created
 * @param[in] lock_id - lock identificator
 * @return error code or 0 on success
 */
int pcilib_init_lock(pcilib_lock_t *lock, pcilib_lock_flags_t flags, const char *lock_id);

/**
 * this function will unref the defined lock. Any subsequent tries to lock it without reinitializaing it will fail.
 * @param[in,out] lock_ctx - the pointer that points to the lock.
 */
void pcilib_free_lock(pcilib_lock_t *lock_ctx);

/**
 * this function gives the identifier name associated to a lock in the kernel space
 * @param[in] loc - pointer to the lock we want the name
 * @return string corresponding to the name
 */
const char *pcilib_lock_get_name(pcilib_lock_t *lock);


/**
 * Increment reference count(number of processes that may access the given lock).
 * Not thread/process safe unless system supports stdatomic (gcc 4.9+). In this case, the access should be synchronized by the caller.
 * @param[in,out] lock - pointer to initialized lock
 */
void pcilib_lock_ref(pcilib_lock_t *lock);

/**
 * Decrement reference count(number of processes that may access the given lock).
 * Not thread/process safe unless system supports stdatomic (gcc 4.9+). In this case, the access should be synchronized by the caller
 * @param[in,out] lock - pointer to initialized lock
 */
void pcilib_lock_unref(pcilib_lock_t *lock);

/**
 * Return _approximate_ number of lock references as the crashed applications will may not unref.
 * @param[in,out] lock - pointer to initialized lock
 * @return the number of lock refs
 */
size_t pcilib_lock_get_refs(pcilib_lock_t *lock);

/**
 * gets the flags associated to the given lock
 * @param[in] lock - the lock we want to know the flags
 * @return value of the flag associated
 */
pcilib_lock_flags_t pcilib_lock_get_flags(pcilib_lock_t *lock);

/**
 * this function will call different locking functions to acquire the given lock. Given the flags, it is thus possible to: 
 * 	1) the process requesting the lock will be held till it can acquire it
 *	2)the lock is tried to be acquired, if the lock can be acquired then it is, if not, then the function returns immediatly and the lock is not taken at all
 *	3) same than previous, but it's possible to define a waiting time to acquire the lock before returning
 *
 * @param[in] lock - the pointer to the mutex
 * @param[in] flags - define the type of lock wanted 
 * @param[in] timeout - the waiting time if asked, before the function returns without having obtained the lock, in micro seconds
 * @return error code or 0 for correctness
 */
int pcilib_lock_custom(pcilib_lock_t* lock, pcilib_lock_flags_t flags, pcilib_timeout_t timeout);

/**
 * function to acquire a lock, and wait till the lock can be acquire
 * @param[in] lock - the pointer to the mutex
 * @return error code or 0 for correctness
 */
int pcilib_lock(pcilib_lock_t* lock);

/**
 * this function will try to take a lock for the mutex pointed by lockfunction to acquire a lock, but that returns immediatly if the lock can't be acquired on first try
 * @param[in] lock - the pointer to the mutex
 * @return error code or 0 for correctness
 */
int pcilib_try_lock(pcilib_lock_t* lock);


/**
  * this function unlocks the lock pointed by lock 
 * @param[in] lock - the integer that points to the semaphore
 */
void pcilib_unlock(pcilib_lock_t* lock);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_LOCK_H */
