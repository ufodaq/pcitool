#define _GNU_SOURCE
#define _XOPEN_SOURCE 600

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#ifdef HAVE_STDATOMIC_H
# include <stdatomic.h>
#endif /* HAVE_STDATOMIC_H */

#include "error.h"
#include "lock.h"
#include "pci.h"

/**
 * structure to define a lock
 */
struct pcilib_lock_s {
  pthread_mutex_t mutex; 		/**< the pthread robust mutex */
  pcilib_lock_flags_t flags; 		/**< flags to define the type of the mutex */
#ifdef HAVE_STDATOMIC_H
  volatile atomic_uint refs; 		/**< approximate number of processes that hold the lock initialized, may desynchronize on crashes */ 
#else /* HAVE_STDATOMIC_H */
    volatile uint32_t refs;		/**< approximate number of processes that hold the lock initialized, may desynchronize on crashes */
#endif /* HAVE_STDATOMIC_H */
  char name[]; 				/**< lock identifier */
};


int pcilib_init_lock(pcilib_lock_t *lock, pcilib_lock_flags_t flags, const char *lock_id) {
    int err;
    pthread_mutexattr_t attr;

    assert(lock);
    assert(lock_id);

    memset(lock, 0, PCILIB_LOCK_SIZE);

    if (strlen(lock_id) >= (PCILIB_LOCK_SIZE - offsetof(struct pcilib_lock_s, name))) {
	pcilib_error("The supplied lock id (%s) is too long...", lock_id);
	return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    if ((err = pthread_mutexattr_init(&attr))!=0) {
	pcilib_error("Can't initialize mutex attribute, errno %i", errno);
	return PCILIB_ERROR_FAILED;
    }

	/* we declare the mutex as possibly shared amongst different processes*/
    if ((err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))!=0) {
	pcilib_error("Can't configure a shared mutex attribute, errno %i", errno);
	return PCILIB_ERROR_FAILED;
    }

	/* we set the mutex as robust, so it would be automatically unlocked if the application crash*/
    if ((flags&PCILIB_LOCK_FLAG_PERSISTENT)==0) {
	if ((err = pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST))!=0) {
	    pcilib_error("Can't configure a robust mutex attribute, errno: %i", errno);
	    return PCILIB_ERROR_FAILED;
	}
    }

    if ((err = pthread_mutex_init(&lock->mutex, &attr))!=0) {
	pcilib_error("Can't create mutex, errno : %i",errno);
	return PCILIB_ERROR_FAILED;
    }

    strcpy(lock->name, lock_id);
    lock->refs = 1;
    lock->flags = flags;

    return 0;
}


void pcilib_free_lock(pcilib_lock_t *lock) {
    int err;

    assert(lock);

//    if (lock->refs)
//	pcilib_error("Forbidding to destroy the referenced mutex...");

    if ((err = pthread_mutex_destroy(&lock->mutex))==-1)
	pcilib_warning("Can't destroy POSIX mutex, errno %i",errno);
}


void pcilib_lock_ref(pcilib_lock_t *lock) {
    assert(lock);
    
#ifdef HAVE_STDATOMIC_H
    atomic_fetch_add_explicit(&lock->refs, 1, memory_order_relaxed);
#else /* HAVE_STDATOMIC_H */
    lock->refs++;
#endif  /* HAVE_STDATOMIC_H */
}

void pcilib_lock_unref(pcilib_lock_t *lock) {
    assert(lock);

    if (!lock->refs) {
	pcilib_warning("Lock is not referenced");
	return;
    }

#ifdef HAVE_STDATOMIC_H
    atomic_fetch_sub_explicit(&lock->refs, 1, memory_order_relaxed);
#else /* HAVE_STDATOMIC_H */
    lock->refs--;
#endif  /* HAVE_STDATOMIC_H */
}

size_t pcilib_lock_get_refs(pcilib_lock_t *lock) {
    return lock->refs;
}

pcilib_lock_flags_t pcilib_lock_get_flags(pcilib_lock_t *lock) {
    return lock->flags;
}

const char *pcilib_lock_get_name(pcilib_lock_t *lock) {
    assert(lock);

    if (lock->name[0]) return lock->name;
    return NULL;
}

int pcilib_lock_custom(pcilib_lock_t *lock, pcilib_lock_flags_t flags, pcilib_timeout_t timeout) {
    int err;

    if (!lock) {
	pcilib_error("The null lock pointer is passed to lock function");
	return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    struct timespec tm;

    switch (timeout) {
     case PCILIB_TIMEOUT_INFINITE:
	/* the process will be hold till it can gain acquire the lock*/
        err = pthread_mutex_lock(&lock->mutex);
	break;
     case PCILIB_TIMEOUT_IMMEDIATE:
	/* the function returns immediatly if it can't acquire the lock*/
        err = pthread_mutex_trylock(&lock->mutex);
	break;
     default:
	/* the process will be hold till it can acquire the lock and timeout is not reached*/
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_nsec += 1000 * (timeout%1000000);
        if (tm.tv_nsec < 1000000000)
	    tm.tv_sec += timeout/1000000;
	else {
	    tm.tv_sec += 1 + timeout/1000000;
	    tm.tv_nsec -= 1000000000;
        } 
        err = pthread_mutex_timedlock(&lock->mutex, &tm);
    }

    if (!err)
	return 0;

    switch (err) {
     case EOWNERDEAD:
	/*in the case an application with a lock acquired crashes, this lock becomes inconsistent. we have so to make it consistent again to use it again.*/
        err = pthread_mutex_consistent(&lock->mutex);
        if (err) {
	    pcilib_error("Failed to mark mutex as consistent, errno %i", err);
	    break;
        }
        return 0;
     case ETIMEDOUT:
     case EBUSY:
        return PCILIB_ERROR_TIMEOUT;
     default:
        pcilib_error("Failed to obtain mutex, errno %i", err);
    }

    return PCILIB_ERROR_FAILED;
}

int pcilib_lock(pcilib_lock_t* lock) {
    return pcilib_lock_custom(lock, PCILIB_LOCK_FLAGS_DEFAULT, PCILIB_TIMEOUT_INFINITE);
}

int pcilib_try_lock(pcilib_lock_t* lock) {
    return pcilib_lock_custom(lock, PCILIB_LOCK_FLAGS_DEFAULT, PCILIB_TIMEOUT_IMMEDIATE);
}

void pcilib_unlock(pcilib_lock_t *lock) {
    int err;

    if (!lock)
	return;

    if ((err = pthread_mutex_unlock(&lock->mutex)) != 0) {
	switch (err) {
	 case EPERM:
	    pcilib_error("Trying to unlock not locked mutex (%s) or the mutex which was locked by a different thread", lock->name);
	    break;
	 default:
	    pcilib_error("Can't unlock mutex, errno %i", err);
	}
    }
}
