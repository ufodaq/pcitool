#ifndef _PCILIB_DEBUG_H
#define _PCILIB_DEBUG_H

#include <stdarg.h>
#include <pcilib/env.h>
#include <pcilib/error.h>

#define PCILIB_DEBUG

#ifdef PCILIB_DEBUG
# define PCILIB_DEBUG_DMA
# define PCILIB_DEBUG_MISSING_EVENTS
# define PCILIB_DEBUG_VIEWS
#endif /* PCILIB_DEBUG */


#ifdef PCILIB_DEBUG_DMA
# define PCILIB_DEBUG_DMA_MESSAGE(function, ...) if (pcilib_getenv(function##_ENV, #function)) { pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__); }
# define PCILIB_DEBUG_DMA_BUFFER(function, ...) if (pcilib_getenv(function##_ENV, #function)) { pcilib_debug_data_buffer (#function, __VA_ARGS__); }
#else /* PCILIB_DEBUG_DMA */
# define PCILIB_DEBUG_DMA_MESSAGE(function, ...)
# define PCILIB_DEBUG_DMA_BUFFER(function, ...)
#endif /* PCILIB_DEBUG_DMA */

#ifdef PCILIB_DEBUG_MISSING_EVENTS
# define PCILIB_DEBUG_MISSING_EVENTS_MESSAGE(function, ...) if (pcilib_getenv(function##_ENV, #function)) { pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__); }
# define PCILIB_DEBUG_MISSING_EVENTS_BUFFER(function, ...) if (pcilib_getenv(function##_ENV #function)) { pcilib_debug_data_buffer (#function, __VA_ARGS__); }
#else /* PCILIB_DEBUG_MISSING_EVENTS */
# define PCILIB_DEBUG_MISSING_EVENTS_MESSAGE(function, ...)
# define PCILIB_DEBUG_MISSING_EVENTS_BUFFER(function, ...)
#endif /* PCILIB_DEBUG_MISSING_EVENTS */

#ifdef PCILIB_DEBUG_VIEWS
# define PCILIB_DEBUG_VIEWS_MESSAGE(function, ...) if (pcilib_getenv(function##_ENV, #function)) { pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__); }
# define PCILIB_DEBUG_VIEWS_BUFFER(function, ...) if (pcilib_getenv(function##_ENV #function)) { pcilib_debug_data_buffer (#function, __VA_ARGS__); }
#else /* PCILIB_DEBUG_VIEWS */
# define PCILIB_DEBUG_VIEWS_MESSAGE(function, ...)
# define PCILIB_DEBUG_VIEWS_BUFFER(function, ...)
#endif /* PCILIB_DEBUG_VIEWS */

#define pcilib_debug(function, ...) \
    PCILIB_DEBUG_##function##_MESSAGE(PCILIB_DEBUG_##function, PCILIB_LOG_DEFAULT, __VA_ARGS__)

#define pcilib_debug_buffer(function, ...) \
    PCILIB_DEBUG_##function##_BUFFER(PCILIB_DEBUG_##function, __VA_ARGS__)


typedef enum {
    PCILIB_DEBUG_BUFFER_APPEND = 1,
    PCILIB_DEBUG_BUFFER_MKDIR = 2
} pcilib_debug_buffer_flags_t; 


#ifdef __cplusplus
extern "C" {
#endif

void pcilib_debug_message(const char *function, const char *file, int line, pcilib_log_flags_t flags, const char *format, ...);
void pcilib_debug_data_buffer(const char *function, size_t size, void *buffer, pcilib_debug_buffer_flags_t flags, const char *file, ...);


#ifdef __cplusplus
}
#endif


#endif /* _PCILIB_DEBUG_H */
