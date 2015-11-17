#ifndef _PCILIB_TIMING_H
#define _PCILIB_TIMING_H

#include <sys/time.h>
#include <pcilib.h>

#ifdef __cplusplus
extern "C" {
#endif

int pcilib_add_timeout(struct timeval *tv, pcilib_timeout_t timeout);
int pcilib_calc_deadline(struct timeval *tv, pcilib_timeout_t timeout);
int pcilib_check_deadline(struct timeval *tve, pcilib_timeout_t timeout);
pcilib_timeout_t pcilib_calc_time_to_deadline(struct timeval *tve);
int pcilib_sleep_until_deadline(struct timeval *tv);
int pcilib_timecmp(struct timeval *tv1, struct timeval *tv2);
pcilib_timeout_t pcilib_timediff(struct timeval *tve, struct timeval *tvs);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_TIMING_H */
