#define _XOPEN_SOURCE

#include "time_fmt.h"
#include "util.h"

#include <time.h>

char* gmel_strptime(char* func_name, int argc, const char* argv[]) {
    const char* date_str = argv[1];
    const char* format = argv[0];
    struct tm tm;
    time_t epoch;
    char* err_msg = "";
    char* result = NULL;

    memset(&tm, 0, sizeof(struct tm));

    if (!strptime(date_str, format, &tm)) {
        err_msg = safe_sprintf("$(error %s: format '%s' failed on parse '%s')",
                               func_name, format, date_str);
        gmk_expand(err_msg);
        perror(err_msg);
        abort();
        return NULL;
    }

    epoch = mktime(&tm);

    result = (char*)GMEL_ALLOC(256);
    sprintf(result, "%lu", (long unsigned)epoch);
    return result;
}

char* gmel_strftime(char* func_name, int argc, const char* argv[]) {
    const char* format = argv[0];
    const char* timestamp = argv[1];
    char* err_msg = "";
    struct tm tm;

    size_t result_size = 256;
    char* result = NULL;

    memset(&tm, 0, sizeof(struct tm));

    if (!strptime(timestamp, "%s", &tm)) {
        err_msg =
            safe_sprintf("$(error %s: strptime '%%s' failed on parse '%s')",
                         func_name, timestamp);
        gmk_expand(err_msg);
        perror(err_msg);
        abort();
        return NULL;
    }

    result = (char*)GMEL_ALLOC(result_size);

    if (!strftime(result, result_size, format, &tm)) {
        err_msg = safe_sprintf("$(error %s: strftime failed on '%s')",
                               func_name, format);
        gmk_expand(err_msg);
        perror(err_msg);
        abort();
        return NULL;
    }

    return result;
}

char* gmel_strfptime(char* func_name, int argc, const char* argv[]) {
    const char* dst_format = argv[0];
    const char* src_format = argv[1];
    const char* date_str = argv[2];

    char* err_msg = "";
    struct tm tm;

    size_t result_size = 256;
    char* result = NULL;

    memset(&tm, 0, sizeof(struct tm));

    if (!strptime(date_str, src_format, &tm)) {
        err_msg =
            safe_sprintf("$(error %s: strptime '%s' failed on parse '%s')",
                         func_name, src_format, date_str);
        gmk_expand(err_msg);
        perror(err_msg);
        abort();
        return NULL;
    }

    result = (char*)GMEL_ALLOC(result_size);

    if (!strftime(result, result_size, dst_format, &tm)) {
        err_msg = safe_sprintf("$(error %s: strftime failed on '%s')",
                               func_name, dst_format);
        gmk_expand(err_msg);
        perror(err_msg);
        abort();
        return NULL;
    }

    return result;
}
