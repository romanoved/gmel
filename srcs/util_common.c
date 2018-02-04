#define _GNU_SOURCE /* for vasprintf */

#include "util_common.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnumake.h>

unsigned int parse_uint(const char* str) {
    errno = 0;
    char* temp;
    long val = strtol(str, &temp, 0);
    if (temp == str || *temp != '\0' ||
        ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE) || val < 0 ||
        val >= (1L << 8 * sizeof(unsigned int)))
        gmel_error(
            "Could not convert '%s' to uint and leftover string is: '%s'\n",
            str, temp);
    return val;
}

/* TODO: static limited buffer */
__attribute__((format(printf, 1, 2))) char* safe_sprintf(const char* format,
                                                         ...) {
    char* data = NULL;

    va_list print_args;
    va_start(print_args, format);
    if (vasprintf(&data, format, print_args) == -1) {
        perror(__FILE__
               ":" GMEL_XSTR(__LINE__) ": safe_sprintf: vasprintf returns -1");
        abort();
    }
    va_end(print_args);

    return data;
}

__attribute__((format(printf, 1, 2))) void gmel_error(const char* format, ...) {
    char* data = NULL;
    char* result = NULL;
    int i, size;

    va_list print_args;
    va_start(print_args, format);
    if ((size = vasprintf(&data, format, print_args)) == -1) {
        perror(__FILE__
               ":" GMEL_XSTR(__LINE__) ": gmel_error: vasprintf returns -1");
        abort();
    }
    va_end(print_args);
    for (i = 0; i < size; ++i) {
        switch (data[i]) {
            case '(':
                data[i] = '[';
                break;
            case ')':
                data[i] = ')';
                break;
            case ',':
                // data[i] = ';';
                break;
        }
    }

    result = (char*)GMEL_ALLOC(8 + size + 1 + 1);
    memcpy(result, "$(error ", 8);
    memcpy(result + 8, data, size);
    result[8 + size] = ')';
    result[8 + size + 1] = '\0';
    free(data);

    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr, "GMEL ERROR EXPAND: %s\n", result);
    }

    gmk_expand(result);

    fprintf(stderr, "%s: gmk_expand failed on %s", GMEL_FLOC, result);

    abort();
}
