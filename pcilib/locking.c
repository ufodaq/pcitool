#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdint.h>
#include <sys/file.h>

#include "locking.h"
#include "error.h"
#include "pci.h"
#include "kmem.h"

/*
 * this function allocates the kernel memory for the locks for software registers
 */
int pcilib_init_locking(pcilib_t* ctx) {
    int i;
    int err;
    pcilib_kmem_reuse_state_t reused;

    assert(PCILIB_LOCK_PAGES * PCILIB_KMEM_PAGE_SIZE >= PCILIB_MAX_LOCKS * PCILIB_LOCK_SIZE);
	
	/*protection against multiple creations of kernel space*/
    err = pcilib_lock_global(ctx);
    if (err) return err;

	/* by default, this kernel space is persistent and will be reused, in order to avoid the big initialization times for robust mutexes each time we run pcitool*/
    ctx->locks.kmem = pcilib_alloc_kernel_memory(ctx, PCILIB_KMEM_TYPE_PAGE, PCILIB_LOCK_PAGES, PCILIB_KMEM_PAGE_SIZE, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_LOCKS,0), PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
    if (!ctx->locks.kmem) {
	pcilib_unlock_global(ctx);
	pcilib_error("Allocation of kernel memory for locking subsystem has failed");
	return PCILIB_ERROR_FAILED;
    }

    reused = pcilib_kmem_is_reused(ctx, ctx->locks.kmem);
    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
	pcilib_unlock_global(ctx);
        pcilib_error("Inconsistent kernel memory for locking subsystem is found (only part of the required buffers is available)");
        return PCILIB_ERROR_INVALID_STATE;
    }

    if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
        for (i = 0; i < PCILIB_LOCK_PAGES; i++) {
	    void *addr = pcilib_kmem_get_block_ua(ctx, ctx->locks.kmem, i);
	    memset(addr, 0, PCILIB_KMEM_PAGE_SIZE);
        }
    }

	/* the lock that has been used for the creation of kernel space is declared unlocked, has we shouldnot use it anymore*/
    ctx->locks.locking = pcilib_get_lock(ctx, PCILIB_LOCK_FLAG_UNLOCKED, "locking");

    pcilib_unlock_global(ctx);

    if ((!ctx->locks.locking)) {
	pcilib_error("Locking subsystem has failed to initialized mandatory global locks");
	return PCILIB_ERROR_FAILED;
    }

    return 0;
}

/*
 * this function free the kernel memory allocated for them and destroys locks by setting memory to 0
 */
void pcilib_free_locking(pcilib_t *ctx) {
    if (ctx->locks.locking)
	pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, ctx->locks.locking);

    if (ctx->locks.kmem) {
	pcilib_free_kernel_memory(ctx, ctx->locks.kmem, PCILIB_KMEM_FLAG_REUSE);
    }

    memset(&ctx->locks, 0, sizeof(pcilib_locking_t));
}

int pcilib_lock_global(pcilib_t *ctx) {
    int err;
    
    /* we flock() on the board's device file to make sure to not have two initialization in the same time (possible long time to init) */
    if ((err = flock(ctx->handle, LOCK_EX))==-1) {
	pcilib_error("Can't get flock on device file");
	return PCILIB_ERROR_FAILED;
    }

    return 0;
}

void pcilib_unlock_global(pcilib_t *ctx) {
    if (flock(ctx->handle, LOCK_UN) == -1)
	pcilib_warning("Could not correctly remove lock from the device file");
}

pcilib_lock_t *pcilib_get_lock_by_id(pcilib_t *ctx, pcilib_lock_id_t id) {
    int page = id / PCILIB_LOCKS_PER_PAGE;
    int offset = id - page * PCILIB_LOCKS_PER_PAGE;
    void *addr = pcilib_kmem_get_block_ua(ctx, ctx->locks.kmem, page);
    pcilib_lock_t *lock = (pcilib_lock_t*)(addr + offset * PCILIB_LOCK_SIZE);

    return lock;
}

pcilib_lock_t *pcilib_get_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, const char  *lock_id, ...) {
    pcilib_lock_id_t i;
    int err, ret;

    pcilib_lock_t *lock;
    char buffer[PCILIB_LOCK_SIZE];

	/* we construct the complete lock_id given the parameters of the function*/
    va_list pa;
    va_start(pa, lock_id);
    ret = vsnprintf(buffer, PCILIB_LOCK_SIZE, lock_id, pa);
    va_end(pa);

    if (ret < 0) {
	pcilib_error("Failed to construct the lock id, probably arguments does not match the format string (%s)...", lock_id);
	return NULL;
    }
	
	
	/* we iterate through locks to see if there is one already with the same name*/	
	// Would be nice to have hash here
    for (i = 0; i < PCILIB_MAX_LOCKS; i++) {
	lock = pcilib_get_lock_by_id(ctx, i);

        const char *name = pcilib_lock_get_name(lock);
	if (!name) break;
	
	if (!strcmp(buffer, name)) {
	    if ((pcilib_lock_get_flags(lock)&PCILIB_LOCK_FLAG_PERSISTENT) != (flags&PCILIB_LOCK_FLAG_PERSISTENT)) {
		if (flags&PCILIB_LOCK_FLAG_PERSISTENT)
		    pcilib_error("Requesting persistent lock (%s), but requested lock is already existing and is robust", name);
		else
		    pcilib_error("Requesting robust lock (%s), but requested lock is already existing and is persistent", name);
		return NULL;
	    }

#ifndef HAVE_STDATOMIC_H
	    if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0) {
		err = pcilib_lock(ctx->locks.locking);
		if (err) {
		    pcilib_error("Error (%i) obtaining global lock", err);
		    return NULL;
		}
	    }
#endif /* ! HAVE_STDATOMIC_H */
	/* if yes, we increment its ref variable*/
	    pcilib_lock_ref(lock);
#ifndef HAVE_STDATOMIC_H
	    if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
		pcilib_unlock(ctx->locks.locking);
#endif /* ! HAVE_STDATOMIC_H */

	    return lock;
	}
    }

    if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0) {
	err = pcilib_lock(ctx->locks.locking);
	if (err) {
	    pcilib_error("Error (%i) obtaining global lock", err);
	    return NULL;
	}
    }

	// Make sure it was not allocated meanwhile
    for (; i < PCILIB_MAX_LOCKS; i++) {
	lock = pcilib_get_lock_by_id(ctx, i);

        const char *name = pcilib_lock_get_name(lock);
	if (!name) break;

	if (!strcmp(buffer, name)) {
	    if ((pcilib_lock_get_flags(lock)&PCILIB_LOCK_FLAG_PERSISTENT) != (flags&PCILIB_LOCK_FLAG_PERSISTENT)) {
		if (flags&PCILIB_LOCK_FLAG_PERSISTENT)
		    pcilib_error("Requesting persistent lock (%s), but requested lock is already existing and is robust", name);
		else
		    pcilib_error("Requesting robust lock (%s), but requested lock is already existing and is persistent", name);
		
		if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
		    pcilib_unlock(ctx->locks.locking);
		return NULL;
	    }

	    pcilib_lock_ref(lock);
	    if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
		pcilib_unlock(ctx->locks.locking);
	    return lock;
	}
    }

    if (i == PCILIB_MAX_LOCKS) {
	if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
	    pcilib_unlock(ctx->locks.locking);
	pcilib_error("Failed to create lock (%s), only %u locks is supported", buffer, PCILIB_MAX_LOCKS);
	return NULL;
    }

	/* if the lock did not exist before, then we create it*/
    err = pcilib_init_lock(lock, flags, buffer);
    
    if (err) {
	pcilib_error("Lock initialization failed with error %i", err);

	if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
	    pcilib_unlock(ctx->locks.locking);
	
	return NULL;
    }
    
    if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
	pcilib_unlock(ctx->locks.locking);

    return lock;
}

void pcilib_return_lock(pcilib_t *ctx, pcilib_lock_flags_t flags, pcilib_lock_t *lock) {
#ifndef HAVE_STDATOMIC_H
	int err;
	
	if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0) {
	    err = pcilib_lock(ctx->locks.locking);
	    if (err) {
		pcilib_error("Error (%i) obtaining global lock", err);
		return;
	    }
	}
#endif /* ! HAVE_STDATOMIC_H */
	pcilib_lock_unref(lock);
#ifndef HAVE_STDATOMIC_H
	if ((flags&PCILIB_LOCK_FLAG_UNLOCKED)==0)
	    pcilib_unlock(ctx->locks.locking);
#endif /* ! HAVE_STDATOMIC_H */
}


/*
 * Destroy all existing locks. This is unsafe call as this and other running applications
 * will still have all initialized lock pointers. It is user responsibility to issue this
 * command when no other application is running.
 */
int pcilib_destroy_all_locks(pcilib_t *ctx, int force) {
    int err;
    pcilib_lock_id_t i;
    pcilib_kmem_reuse_state_t reused;

    if (strcasecmp(ctx->model, "maintenance")) {
	pcilib_error("Can't destroy locks while locking subsystem is initialized, use maintenance model");
	return PCILIB_ERROR_INVALID_STATE;
    }

    err = pcilib_lock_global(ctx);
    if (err) return err;

	// ToDo: We should check here that no other instances of pcitool are running, the driver can provide this information

    ctx->locks.kmem = pcilib_alloc_kernel_memory(ctx, PCILIB_KMEM_TYPE_PAGE, PCILIB_LOCK_PAGES, PCILIB_KMEM_PAGE_SIZE, 0, PCILIB_KMEM_USE(PCILIB_KMEM_USE_LOCKS,0), PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
    if (!ctx->locks.kmem) {
	pcilib_unlock_global(ctx);
	pcilib_error("Failed to allocate kernel memory of locking subsystem");
	return PCILIB_ERROR_FAILED;
    }

    reused = pcilib_kmem_is_reused(ctx, ctx->locks.kmem);
    if (reused & PCILIB_KMEM_REUSE_PARTIAL) {
	pcilib_unlock_global(ctx);
        pcilib_error("Inconsistent kernel memory for locking subsystem is found (only part of the required buffers is available)");
        return PCILIB_ERROR_INVALID_STATE;
    }

    if ((reused & PCILIB_KMEM_REUSE_REUSED) == 0) {
	pcilib_free_kernel_memory(ctx, ctx->locks.kmem, PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_PERSISTENT);
	pcilib_unlock_global(ctx);
	return 0;
    }

	/* if we run in non-forced case, then if it may be still processes that can have access to the locks, they are not destroyed*/
    if (!force) {
	for (i = 0; i < PCILIB_MAX_LOCKS; i++) {
	    pcilib_lock_t *lock = pcilib_get_lock_by_id(ctx, i);

	    const char *name = pcilib_lock_get_name(lock);
	    if (!name) break;
	
	    size_t refs = pcilib_lock_get_refs(lock);

	    if (refs > 0) {
		char *stmp = strdup(name);
		pcilib_free_locking(ctx);
		pcilib_unlock_global(ctx);
		pcilib_error("Lock (%s) has %zu references, destroying references may result in crashes and data corruption", stmp, refs);
		free(stmp);
		return PCILIB_ERROR_BUSY;
	    }
	}
    }

	// Do we really need this? I guess zeroing should be enough
    for (i = 0; i < PCILIB_MAX_LOCKS; i++) {
	pcilib_lock_t *lock = pcilib_get_lock_by_id(ctx, i);

	const char *name = pcilib_lock_get_name(lock);
	if (!name) break;

	pcilib_free_lock(lock);
    }

    for (i = 0; i < PCILIB_LOCK_PAGES; i++) {
	void *addr = pcilib_kmem_get_block_ua(ctx, ctx->locks.kmem, i);
	memset(addr, 0, PCILIB_KMEM_PAGE_SIZE);
    }

    pcilib_free_locking(ctx);
    pcilib_unlock_global(ctx);

    return 0;
}
