#include "bind.h"
#include "popen.h"
#include "time_fmt.h"

#include "util.h"

#include <gnumake.h>

int plugin_is_GPL_compatible;

int gmel_gmk_setup() {
    gmk_add_function("strptime", (gmk_func_ptr)gmel_strptime, 2, 2, 0);
    gmk_add_function("strftime", (gmk_func_ptr)gmel_strftime, 2, 2, 0);
    gmk_add_function("strfptime", (gmk_func_ptr)gmel_strfptime, 3, 3, 0);

    gmk_add_function("popen", (gmk_func_ptr)gmel_popen, 1, 0, 0);

    if (!gmel_memfunc_table_setup()) return 0;

    gmk_add_function("bind", (gmk_func_ptr)gmel_bind, 2, 0, GMK_FUNC_NOEXPAND);
    gmk_add_function("bind_r", (gmk_func_ptr)gmel_bind_r, 2, 0,
                     GMK_FUNC_NOEXPAND);

    gmk_free(gmk_expand("$(bind sshell,popen,/bin/bash,-e,-o,pipefail,-c)"));

    return 1;
}
