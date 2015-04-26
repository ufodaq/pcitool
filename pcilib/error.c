#include <stdio.h>
#include <stdarg.h>

#include "export.h"
#include "error.h"

void pcilib_print_error(const char *msg, ...) {
    va_list va;
    
    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
    printf("\n");
}


int pcilib_set_error_handler(void (*err)(const char *msg, ...), void (*warn)(const char *msg, ...)) {
    if (err) pcilib_error = err;
    else pcilib_error = pcilib_print_error;
    if (warn) pcilib_warning = warn;
    else pcilib_warning = pcilib_print_error;

    return 0;
}
