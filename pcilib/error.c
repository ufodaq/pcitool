#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "export.h"
#include "error.h"

void pcilib_print_error(void *arg, const char *file, int line, pcilib_log_priority_t prio, const char *msg, va_list va) {
    vprintf(msg, va);
    printf(" [%s:%d]\n", file, line);
}

static void *pcilib_logger_argument = NULL;
static pcilib_log_priority_t pcilib_logger_min_prio = PCILIB_LOG_WARNING;
static pcilib_logger_t pcilib_logger = pcilib_print_error;



void pcilib_log_message(const char *file, int line, pcilib_log_priority_t prio, const char *msg, ...) {
    va_list va;

    if (prio >= pcilib_logger_min_prio) {
	va_start(va, msg);
	pcilib_logger(pcilib_logger_argument, file, line, prio, msg, va);
	va_end(va);
    }
}


int pcilib_set_logger(pcilib_log_priority_t min_prio, pcilib_logger_t logger, void *arg) {
    pcilib_logger_min_prio = min_prio;
    pcilib_logger_argument = arg;

    if (logger) pcilib_logger = logger;
    else logger = pcilib_print_error;

    return 0;
}
