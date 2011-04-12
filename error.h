#ifndef _PCILIB_ERROR_H
#define _PCILIB_ERROR_H
 
enum {
    PCILIB_ERROR_SUCCESS = 0,
    PCILIB_ERROR_MEMORY,
    PCILIB_ERROR_INVALID_ADDRESS,
    PCILIB_ERROR_INVALID_BANK,
    PCILIB_ERROR_INVALID_DATA,
    PCILIB_ERROR_TIMEOUT,
    PCILIB_ERROR_FAILED,
    PCILIB_ERROR_VERIFY,
    PCILIB_ERROR_NOTSUPPORTED,
    PCILIB_ERROR_NOTFOUND,
    PCILIB_ERROR_OUTOFRANGE,
    PCILIB_ERROR_NOTAVAILABLE,
    PCILIB_ERROR_NOTINITIALIZED
} pcilib_errot_t;


#endif /* _PCILIB_ERROR_H */
