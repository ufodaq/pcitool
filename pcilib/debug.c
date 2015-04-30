#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

void pcilib_debug_message(const char *function, const char *file, int line, const char *format, ...) {
    va_list va;

    if (!getenv(function)) return;

    va_start(va, format);
    pcilib_log_vmessage(file, line, PCILIB_LOG_DEBUG, format, va);
    va_end(va);
}

