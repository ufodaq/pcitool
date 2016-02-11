#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "export.h"
#include "error.h"

#define PCILIB_LOGGER_HISTORY 16

void pcilib_print_error(void *arg, const char *file, int line, pcilib_log_priority_t prio, const char *msg, va_list va) {
    vprintf(msg, va);
    printf(" [%s:%d]\n", file, line);
}

static void *pcilib_logger_argument = NULL;
static pcilib_log_priority_t pcilib_logger_min_prio = PCILIB_LOG_WARNING;
static pcilib_logger_t pcilib_logger = pcilib_print_error;
static char *pcilib_logger_history[PCILIB_LOGGER_HISTORY] = {0};
static int pcilib_logger_history_pointer = 0;

static int pcilib_log_check_history(pcilib_log_flags_t flags, const char *msg) {
    int i;

    if ((flags&PCILIB_LOG_ONCE) == 0)
	return 0;

    for (i = 0; ((i < PCILIB_LOGGER_HISTORY)&&(pcilib_logger_history[i])); i++) {
	if (!strcmp(msg, pcilib_logger_history[i]))
	    return -1;
    }

    if (pcilib_logger_history[pcilib_logger_history_pointer])
	free(pcilib_logger_history[pcilib_logger_history_pointer]);

    pcilib_logger_history[pcilib_logger_history_pointer] = strdup(msg);

    if (++pcilib_logger_history_pointer == PCILIB_LOGGER_HISTORY)
	pcilib_logger_history_pointer = 0;

    return 0;
}

void pcilib_log_message(const char *file, int line, pcilib_log_flags_t flags, pcilib_log_priority_t prio, const char *msg, ...) {
    va_list va;

    if ((!prio)||(prio >= pcilib_logger_min_prio)) {
	if (pcilib_log_check_history(flags, msg))
	    return;

	va_start(va, msg);
	pcilib_logger(pcilib_logger_argument, file, line, prio, msg, va);
	va_end(va);
    }
}

void pcilib_log_vmessage(const char *file, int line, pcilib_log_flags_t flags, pcilib_log_priority_t prio, const char *msg, va_list va) {
    if ((!prio)||(prio >= pcilib_logger_min_prio)) {
	if (pcilib_log_check_history(flags, msg))
	    return;

	pcilib_logger(pcilib_logger_argument, file, line, prio, msg, va);
    }
}


int pcilib_set_logger(pcilib_log_priority_t min_prio, pcilib_logger_t logger, void *arg) {
    pcilib_logger_min_prio = min_prio;
    pcilib_logger_argument = arg;

    if (logger) pcilib_logger = logger;
    else logger = pcilib_print_error;

    return 0;
}

pcilib_logger_t pcilib_get_logger() {
    return pcilib_logger;
}
pcilib_log_priority_t pcilib_get_logger_min_prio() {
	return pcilib_logger_min_prio;
}
void* pcilib_get_logger_argument() {
	return pcilib_logger_argument;
}

