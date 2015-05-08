#ifndef _PCILIB_ENV_H
#define _PCILIB_ENV_H

typedef enum {
    PCILIB_DEBUG_DMA_ENV,
    PCILIB_DEBUG_MISSING_EVENTS_ENV,
    PCILIB_MAX_ENV
} pcilib_env_t;

const char *pcilib_getenv(pcilib_env_t env, const char *var);

#endif /* _PCILIB_ENV_H */
