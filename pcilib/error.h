#ifndef _PCILIB_ERROR_H
#define _PCILIB_ERROR_H

#include <errno.h>
#include <pcilib.h>

enum {
    PCILIB_ERROR_SUCCESS = 0,
    PCILIB_ERROR_MEMORY = ENOMEM,
    PCILIB_ERROR_INVALID_REQUEST = EBADR,
    PCILIB_ERROR_INVALID_ADDRESS = EFAULT,
    PCILIB_ERROR_INVALID_BANK = ENOENT,
    PCILIB_ERROR_INVALID_DATA = EILSEQ,
    PCILIB_ERROR_INVALID_STATE =  EBADFD,
    PCILIB_ERROR_INVALID_ARGUMENT = EINVAL,
    PCILIB_ERROR_TIMEOUT = ETIME,
    PCILIB_ERROR_FAILED = EBADE,
    PCILIB_ERROR_VERIFY = EREMOTEIO,
    PCILIB_ERROR_NOTSUPPORTED = ENOTSUP,
    PCILIB_ERROR_NOTFOUND = ESRCH,
    PCILIB_ERROR_OUTOFRANGE = ERANGE,
    PCILIB_ERROR_NOTAVAILABLE = ENAVAIL,
    PCILIB_ERROR_NOTINITIALIZED = EBADFD,
    PCILIB_ERROR_TOOBIG = EFBIG,
    PCILIB_ERROR_OVERWRITTEN = ESTALE,
    PCILIB_ERROR_BUSY = EBUSY
} pcilib_errot_t;

void pcilib_log_message(const char *file, int line, pcilib_log_priority_t prio, const char *msg, ...);
void pcilib_log_vmessage(const char *file, int line, pcilib_log_priority_t prio, const char *msg, va_list va);

#define pcilib_log(prio, ...) \
    pcilib_log_message(__FILE__, __LINE__, prio, __VA_ARGS__)

#define pcilib_error(...)		pcilib_log(PCILIB_LOG_ERROR, __VA_ARGS__)
#define pcilib_warning(...)	pcilib_log(PCILIB_LOG_WARNING, __VA_ARGS__)
#define pcilib_info(...)		pcilib_log(PCILIB_LOG_INFO, __VA_ARGS__)


#endif /* _PCILIB_ERROR_H */
