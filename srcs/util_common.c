#define _GNU_SOURCE /* for vasprintf */

#include "util_common.h"

#include <gnumake.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((format(printf, 1, 2))) char* save_sprintf(const char* format,
                                                         ...) {
    char* data = NULL;

    va_list print_args;
    va_start(print_args, format);
    if (vasprintf(&data, format, print_args) + 1 <= 0) {
        perror("util_common.h::save_sprintf vasprintf fault");
        abort();
    }
    va_end(print_args);

    return data;
}

void expand_escapes(char* dest, const char* src) {
    char c;

    while ((c = *(src++))) {
        switch (c) {
            case '\a':
                *(dest++) = '\\';
                *(dest++) = 'a';
                break;
            case '\b':
                *(dest++) = '\\';
                *(dest++) = 'b';
                break;
            case '\t':
                *(dest++) = '\\';
                *(dest++) = 't';
                break;
            case '\n':
                *(dest++) = '\\';
                *(dest++) = 'n';
                break;
            case '\v':
                *(dest++) = '\\';
                *(dest++) = 'v';
                break;
            case '\f':
                *(dest++) = '\\';
                *(dest++) = 'f';
                break;
            case '\r':
                *(dest++) = '\\';
                *(dest++) = 'r';
                break;
            case '\\':
                *(dest++) = '\\';
                *(dest++) = '\\';
                break;
            case '\"':
                *(dest++) = '\\';
                *(dest++) = '\"';
                break;
            default:
                *(dest++) = c;
        }
    }

    *dest = '\0';
}

char* sprintf_argv(char** argv) {
    char* tmp = NULL;
    char* escaped_arg = NULL;
    char* rez = (char*)malloc(2 * sizeof(char));
    if (!rez) {
        perror("util_common.h::sprintf_argv malloc failed");
        abort();
    }
    rez[0] = '[';
    rez[1] = '\0';

    while (*argv)
        for (; *argv; ++argv) {
            escaped_arg = (char*)malloc(2 * strlen(*argv) + 1);
            if (!escaped_arg) {
                perror("util_common.h::sprintf_argv malloc failed");
                abort();
            }
            expand_escapes(escaped_arg, *argv);
            tmp = save_sprintf("%s'%s'%s", rez, escaped_arg,
                               *(argv + 1) ? ", " : "");
            free(escaped_arg);
            free(rez);
            rez = tmp;
        }
    tmp = save_sprintf("%s]", rez);
    free(rez);
    return tmp;
}

void gmel_abort(char const* msg) {
    perror(msg);
    abort();
}

void* gmel_smalloc(char const* msg, size_t size) {
    void* result = malloc(size);
    if (!result) gmel_abort(msg);
    return result;
}

void gmel_sfree(void* ptr) {
    if (!ptr) return;
    free(ptr);
}
