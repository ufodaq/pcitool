#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <sched.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "pci.h"
#include "tools.h"
#include "error.h"

int pcilib_add_timeout(struct timeval *tv, pcilib_timeout_t timeout) {
    tv->tv_usec += timeout%1000000;
    if (tv->tv_usec > 999999) {
	tv->tv_usec -= 1000000;
	tv->tv_sec += 1 + timeout/1000000;
    } else {
	tv->tv_sec += timeout/1000000;
    }

    return 0;
}

int pcilib_calc_deadline(struct timeval *tv, pcilib_timeout_t timeout) {
    gettimeofday(tv, NULL);
    pcilib_add_timeout(tv, timeout);

    return 0;
}

int pcilib_check_deadline(struct timeval *tve, pcilib_timeout_t timeout) {
    int64_t res;
    struct timeval tvs;

    if (!tve->tv_sec) return 0;

    gettimeofday(&tvs, NULL);
    res = ((tve->tv_sec - tvs.tv_sec)*1000000 + (tve->tv_usec - tvs.tv_usec));
	// Hm... Some problems comparing signed and unsigned. So, sign check first
    if ((res < 0)||(res < timeout)) {
	return 1;
    }

    return 0;
}

pcilib_timeout_t pcilib_calc_time_to_deadline(struct timeval *tve) {
    int64_t res;
    struct timeval tvs;
    
    gettimeofday(&tvs, NULL);
    res = ((tve->tv_sec - tvs.tv_sec)*1000000 + (tve->tv_usec - tvs.tv_usec));
    
    if (res < 0) return 0;
    return res;
}

int pcilib_sleep_until_deadline(struct timeval *tv) {
    struct timespec wait;
    pcilib_timeout_t duration;

    duration = pcilib_calc_time_to_deadline(tv);
    if (duration > 0) {
	wait.tv_sec = duration / 1000000;
	wait.tv_nsec = 1000 * (duration % 1000000);
	nanosleep(&wait, NULL);
    }

    return 0;
}

pcilib_timeout_t pcilib_timediff(struct timeval *tvs, struct timeval *tve) {
    return ((tve->tv_sec - tvs->tv_sec)*1000000 + (tve->tv_usec - tvs->tv_usec));
}

int pcilib_timecmp(struct timeval *tv1, struct timeval *tv2) {
    if (tv1->tv_sec > tv2->tv_sec) return 1;
    else if (tv1->tv_sec < tv2->tv_sec) return -1;
    else if (tv1->tv_usec > tv2->tv_usec) return 1;
    else if (tv1->tv_usec < tv2->tv_usec) return -1;
    return 0;
}
