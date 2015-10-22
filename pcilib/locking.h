/**
 * @file locking.h
 * @brief this file is the header file for functions that touch all locks allocated for software registers.
 * @details for more details about implementation choice, please read the file lock.h
 */
#ifndef _PCILIB_LOCKING_H
#define _PCILIB_LOCKING_H

#define PCILIB_MAX_LOCKS 64										/**< number of maximum locks*/
#define PCILIB_LOCKS_PER_PAGE 	(PCILIB_KMEM_PAGE_SIZE/PCILIB_LOCK_SIZE)				/**< number of locks per page of kernel memory */
#define PCILIB_LOCK_PAGES 	((PCILIB_MAX_LOCKS * PCILIB_LOCK_SIZE)/PCILIB_KMEM_PAGE_SIZE)		/**< number of pages allocated for locks in kernel memory */


#include <pcilib/kmem.h>
#include <pcilib/lock.h>

typedef uint32_t pcilib_lock_id_t; 			/**< type to represent the index of a lock in the table of locks in the kernel space*/

typedef struct pcilib_locking_s pcilib_locking_t;

/**
 * structure defining the kernel space used for locks
 */
struct pcilib_locking_s {
    pcilib_kmem_handle_t *kmem;				/**< kmem used to store mutexes */
    pcilib_lock_t *locking;				/**< lock used while intializing kernel space */
//    pcilib_lock_t *mmap;				/**< lock used to protect mmap operation */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 *this function gets the kernel space for the locks : if this space have been already initialized, then the previous space is returned. If not, the space is created. this function has to be protected, in order to avoid the simultaneous creation of 2 kernel spaces. For that, we use pcilib_lock_global.
 *@param[in,out] ctx - the pcilib_t structure running, getting filled with a ref to locks' kernel space
 */
int pcilib_init_locking(pcilib_t *ctx);

/**
 *this function cleans the memory from all locks : the kernel space is freed, and locks references in pcilib_t are destroyed by setting memory to 0
 *@param[in,out] ctx - the pcilib_t structure running
 */
void pcilib_free_locking(pcilib_t *ctx);

/**
 * this function use flock locking mechanism on the ALPS platform device file, to make sure to not create two kernel spaces for locks
 *@param[in,out] ctx - the pcilib_t structure running
 */
int pcilib_lock_global(pcilib_t *ctx);

/**
 *function to remove the lock created by flock on the ALPS platform device file
 *@param[in,out] ctx - the pcilib_t structure running
 */
void pcilib_unlock_global(pcilib_t *ctx);

/**
 * this function returns the lock at the index in the kernel space equal to id
 *@param[in] ctx - the pcilib_t structure running
 *@param[in] id - the index of the lock
 *@return the lock structure corresponding
 */
pcilib_lock_t *pcilib_get_lock_by_id(pcilib_t *ctx, pcilib_lock_id_t id);

/**
 *this function verify if the lock requested exists in the kernel space. If yes, then nothing is done, else we create the lock in the kernel space. This function also gives the number of processes that may request the lock afterwards, including the one that just created it. 
 *@param[in] ctx - the pcilib_t structure running
 *@param[in] flags - the flag defining the property of the lock
 *@param[in] lock_id - the identifier name for the lock
 *@return the corresponding lock, or a new one if it did not exist before
 */
pcilib_lock_t *pcilib_get_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, const char *lock_id, ...);

/**
 *this function is to decrement the variable in a lock containing the number of processes that may access to this lock(refs)
 *@param[in] ctx - the pcilib_t structure running
 *@param[in] flags - the flag defining the property of the lock
 *@param[in] lock - pointer to the lock we want to modify
 */
void pcilib_return_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, pcilib_lock_t *lock);

/**
 * this function destroy all the locks that have been created(unref the locks + set memory to 0), and so is used when we want to clean properly the kernel space. If force is set to 1, then we don't care about other processes that may request locks. If not, if there is locks that may be requested by other processes, then the operation is stopped. Of course, destroying locks that may be requested by other processes results in an undefined behaviour. Thus, it is user responsibility to issue this command with force set to 1
 *@param[in,out] ctx - the pcilib_t structure running
 *@param[in] force - should the operation be forced or not
 * @return error code : 0 if everything was ok
 */
int pcilib_destroy_all_locks(pcilib_t *ctx, int force);


#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_LOCKING_H */
