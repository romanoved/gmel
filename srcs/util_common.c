#define _GNU_SOURCE /* for vasprintf */

#include "util_common.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnumake.h>

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
                data[i] = ';';
                break;
        }
    }

    result = (char*)GMEL_ALLOC(8 + size + 1 + 1);
    memcpy(result, "$(error ", 8);
    memcpy(result + 8, data, size);
    result[8 + size] = ')';
    result[8 + size + 1] = '\0';
    free(data);

    gmk_expand(result);

    fprintf(stderr, "%s: gmk_expand failed on %s", GMEL_FLOC, result);

    abort();
}
