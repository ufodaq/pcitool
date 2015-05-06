#define _ISOC99_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "error.h"
#include "debug.h"

#define PCILIB_MAX_DEBUG_FILENAME_LENGTH 1023

void pcilib_debug_message(const char *function, const char *file, int line, pcilib_log_flags_t flags, const char *format, ...) {
    va_list va;

    if (!getenv(function)) return;

    va_start(va, format);
    pcilib_log_vmessage(file, line, PCILIB_LOG_DEBUG, flags, format, va);
    va_end(va);
}


void pcilib_debug_data_buffer(const char *function, size_t size, void *buffer, pcilib_debug_buffer_flags_t flags, const char *file, ...) {
    va_list va;

    FILE *f;
    size_t prefix_len;
    const char *prefix;
    char fname[PCILIB_MAX_DEBUG_FILENAME_LENGTH + 1];


    prefix = getenv(function);
    if (!prefix) return;

    if ((!prefix[0])||(prefix[0] == '1'))
	prefix = PCILIB_DEBUG_DIR;

    prefix_len = strlen(prefix);
    if (prefix_len >= PCILIB_MAX_DEBUG_FILENAME_LENGTH)
	return;

    if (prefix_len) {
	strncpy(fname, prefix, PCILIB_MAX_DEBUG_FILENAME_LENGTH + 1);
	fname[prefix_len++] = '/';
    }
    
    fname[PCILIB_MAX_DEBUG_FILENAME_LENGTH] = -1;
    va_start(va, file);
    vsnprintf(fname + prefix_len, PCILIB_MAX_DEBUG_FILENAME_LENGTH + 1 - prefix_len, file, va);
    va_end(va);

	// file name is too long, skipping...
    if (!fname[PCILIB_MAX_DEBUG_FILENAME_LENGTH])
	return;


    if (flags&PCILIB_DEBUG_BUFFER_MKDIR) {
	char *slash;
	if (prefix_len) slash = fname + prefix_len - 1;
	else slash = strchr(fname, '/');

	while (slash) {
	    size_t len;

	    *slash = 0;
	    mkdir(fname, 0755);
	    len = strlen(fname);
	    *slash = '/';

	    slash = strchr(fname + len + 1, '/');
	}	    
    }

    if (flags&PCILIB_DEBUG_BUFFER_APPEND)
	f = fopen(fname, "a+");
    else 
	f = fopen(fname, "w");

    if (!f)
	return;
    
    if (size&&buffer)
	fwrite(buffer, 1, size, f);

    fclose(f);
}
