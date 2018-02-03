#pragma once

__attribute__((format(printf, 1, 2))) char* safe_sprintf(const char* format,
                                                         ...);
__attribute__((format(printf, 1, 2))) void gmel_error(const char* format, ...);

#define _GMEL_STRINGIZE(expr) #expr
#define GMEL_XSTR(expr) _GMEL_STRINGIZE(expr)

#define GMEL_FLOC __FILE__ ":" GMEL_XSTR(__LINE__)
#define GMEL_FLOC_EXPR(expr) \
    GMEL_FLOC ": assertion failed at \"" GMEL_XSTR(expr) "\""

#define GMEL_STR_ERORR(str) \
    (gmk_expand("$(error " GMEL_FLOC ": " str ")"), abort(), 0)

#define GMEL_ALLOC(size)                                             \
    ({                                                               \
        void* __gmk_alloc_result = gmk_alloc((size));                \
        if (!__gmk_alloc_result) GMEL_STR_ERORR("gmk_alloc failed"); \
        __gmk_alloc_result;                                          \
    })
#define GMEL_FREE(ptr) (ptr = ptr ? gmk_free((void*)ptr), NULL : NULL)

#define GMEL_ASSERT(expr)                                           \
    ((void)((expr)                                                  \
                ? 0                                                 \
                : (gmk_expand("$(error " GMEL_FLOC_EXPR(expr) ")"), \
                   fprintf(stderr, GMEL_FLOC_EXPR(expr) "\n"), abort(), 0)))
#define GMEL_PY_ASSERT(expr)                                        \
    ((void)((expr)                                                  \
                ? 0                                                 \
                : (PyErr_Print(),                                   \
                   gmk_expand("$(error " GMEL_FLOC_EXPR(expr) ")"), \
                   fprintf(stderr, GMEL_FLOC_EXPR(expr) "\n"), abort(), 0)))

#define GMEL_SPABORT(expr)                                             \
    ((void)((expr) ? 0                                                 \
                   : (perror("assertion at "__FILE__                   \
                             ":" GMEL_XSTR(__LINE__) ": \"" GMEL_XSTR( \
                                 expr) "\" failed"),                   \
                      abort(), 0)))
