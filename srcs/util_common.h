#pragma once

#include <stddef.h>

__attribute__((format(printf, 1, 2))) char* save_sprintf(const char* format,
                                                         ...);

void expand_escapes(char* dest, const char* src);

char* sprintf_argv(char** argv);

void gmel_abort(char const* msg);
void gmel_sfree(void* ptr);
void* gmel_smalloc(char const* msg, size_t size);
