#include "bind.h"

#include "util_common.h"

#include <gnumake.h>

int plugin_is_GPL_compatible;

int GMEL_TRACE_LEVEL = 0;

char* gmel_set_trace(char* func_name, int argc, char** argv) {
    GMEL_TRACE_LEVEL = parse_uint(argv[0]);
    return NULL;
}

int gmel_gmk_setup() {
    if (!gmel_memfunc_table_setup()) return 0;

    gmk_add_function("bind", (gmk_func_ptr)gmel_bind, 2, 0, GMK_FUNC_NOEXPAND);
    gmk_add_function("bind_r", (gmk_func_ptr)gmel_bind_r, 2, 0,
                     GMK_FUNC_NOEXPAND);
    gmk_add_function("bind_args", (gmk_func_ptr)gmel_bind_args, 4, 0,
                     GMK_FUNC_NOEXPAND);

    gmk_add_function("gmel_mft_register", (gmk_func_ptr)gmel_mft_register, 3, 3,
                     0);
    gmk_add_function("gmel_set_trace", (gmk_func_ptr)gmel_set_trace, 1, 1, 0);
    gmk_add_function("gmel_print_mft", (gmk_func_ptr)gmel_print_mft, 0, 0, 0);

    return 1;
}
