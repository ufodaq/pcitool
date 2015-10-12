#ifndef _PCILIB_ENV_H
#define _PCILIB_ENV_H

typedef enum {
    PCILIB_DEBUG_DMA_ENV,
    PCILIB_DEBUG_MISSING_EVENTS_ENV,
    PCILIB_DEBUG_VIEWS_ENV,
    PCILIB_MAX_ENV
} pcilib_env_t;

#ifdef __cplusplus
extern "C" {
#endif

const char *pcilib_getenv(pcilib_env_t env, const char *var);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_ENV_H */
