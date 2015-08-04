/**
 * @file lock_global.h
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

typedef uint32_t pcilib_lock_id_t;

typedef struct pcilib_locking_s pcilib_locking_t;
struct pcilib_locking_s {
    pcilib_kmem_handle_t *kmem;							/**< kmem used to store mutexes */
    pcilib_lock_t *locking;							/**< lock used while intializing other locks */
    pcilib_lock_t *mmap;							/**< lock used to protect mmap operation */
};

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_init_locking(pcilib_t *ctx);
void pcilib_free_locking(pcilib_t *ctx);

int pcilib_lock_global(pcilib_t *ctx);
void pcilib_unlock_global(pcilib_t *ctx);

pcilib_lock_t *pcilib_get_lock_by_id(pcilib_t *ctx, pcilib_lock_id_t id);

pcilib_lock_t *pcilib_get_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, const char *lock_id, ...);
void pcilib_return_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, pcilib_lock_t *lock);

int pcilib_destroy_all_locks(pcilib_t *ctx, int force);


#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_LOCKING_H */
