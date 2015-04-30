#ifndef _PCILIB_DEBUG_H
#define _PCILIB_DEBUG_H

#define PCILIB_DEBUG

#ifdef PCILIB_DEBUG
# define PCILIB_DEBUG_DMA
# define PCILIB_DEBUG_MISSING_EVENTS
#endif /* PCILIB_DEBUG */


#ifdef PCILIB_DEBUG_DMA
# define PCILIB_DEBUG_DMA_CALL(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
#else /* PCILIB_DEBUG_DMA */
# define PCILIB_DEBUG_DMA_CALL(function, ...)
#endif /* PCILIB_DEBUG_DMA */

#ifdef PCILIB_DEBUG_MISSING_EVENTS
# define PCILIB_DEBUG_MISSING_EVENTS_CALL(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
#else /* PCILIB_DEBUG_MISSING_EVENTS */
# define PCILIB_DEBUG_MISSING_EVENTS_CALL(function, ...)
#endif /* PCILIB_DEBUG_MISSING_EVENTS */


#define pcilib_debug(function, ...) \
    PCILIB_DEBUG_##function##_CALL(PCILIB_DEBUG_##function, __VA_ARGS__)

void pcilib_debug_message(const char *function, const char *file, int line, const char *format, ...);


#endif /* _PCILIB_DEBUG_H */
