#ifndef _PCILIB_TIMING_H
#define _PCILIB_TIMING_H

#include <sys/time.h>
#include <pcilib.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Add the specified number of microseconds to the time stored in \p tv
 * @param[in,out] tv	- timestamp
 * @param[in] timeout	- number of microseconds to add 
 * @return 		- error code or 0 for correctness
 */
int pcilib_add_timeout(struct timeval *tv, pcilib_timeout_t timeout);

/**
 * Computes the deadline by adding the specified number of microseconds to the current timestamp
 * @param[out] tv	- the deadline
 * @param[in] timeout	- number of microseconds to add 
 * @return 		- error code or 0 for correctness
 */
int pcilib_calc_deadline(struct timeval *tv, pcilib_timeout_t timeout);

/**
 * Check if we are within \p timeout microseconds before the specified deadline or already past it
 * @param[in] tv	- the deadline
 * @param[in] timeout	- maximum number of microseconds before deadline
 * @return 		- 1 if we are within \p timeout microseconds before deadline or past it, 0 - otherwise
 */
int pcilib_check_deadline(struct timeval *tv, pcilib_timeout_t timeout);

/**
 * Compute the remaining time to deadline
 * @param[in] tv	- the deadline
 * @return 		- number of microseconds until deadline or 0 if we are already past it
 */
pcilib_timeout_t pcilib_calc_time_to_deadline(struct timeval *tv);

/**
 * Executes sleep until the specified deadline
 * Real-time capabilities are not used. TThe sleep could wake slightly after the specified deadline.
 * @param[in] tv	- the deadline
 * @return 		- error code or 0 for correctness
 */
int pcilib_sleep_until_deadline(struct timeval *tv);

/**
 * Computes the number of microseconds between 2 timestamps.
 * This function expects that \p tve is after \p tvs.
 * @param[in] tve	- the end of the time interval
 * @param[in] tvs	- the beginning of the time interval
 * @return 		- number of microseconds between two timestamps
 */
pcilib_timeout_t pcilib_timediff(struct timeval *tve, struct timeval *tvs);

/**
 * Compares two timestamps
 * @param[in] tv1	- the first timestamp
 * @param[in] tv2	- the second timestamp
 * @return		- 0 if timestamps are equal, 1 if the first timestamp is after the second, or -1 if the second is after the first.
 */
int pcilib_timecmp(struct timeval *tv1, struct timeval *tv2);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_TIMING_H */
