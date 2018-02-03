#include "bind.h"
#include "popen.h"
#include "util.h"

/* dynamically bind popen wrapper */

static HashTable GMEL_MEMFUNC_TABLE;

typedef struct gmel_memfunc {
    int argc;
    char** argv;
    gmk_func_ptr binded_func_ptr;
    gmel_strview binded_func_name;
} gmel_memfunc;

void gmel_memfunc_free(gmel_memfunc* ptr) {
    int i;
    for (i = 0; i < ptr->argc; ++i) GMEL_FREE(ptr->argv[i]);
}

void gmel_memfunc_tabkle_key_free(void* key) {
    gmel_strview_free((gmel_strview*)key);
    free(key);
}

void gmel_memfunc_tabkle_value_free(void* value) {
    gmel_memfunc_free((gmel_memfunc*)value);
    free(value);
}

void vector_push_back_string(Vector* vector, const char* str, size_t size) {
    size_t pos;
    size = size ? size : strlen(str);
    for (pos = 0; pos < size; ++pos)
        GMEL_ASSERT(!vector_push_back(vector, (void*)(str + pos)));
}
char* gmel_memfunc_call(char* func_name, int argc, char** argv) {
    int i;

    int tmp_argc = 0;
    char** tmp_argv = NULL;

    char* result = NULL;

    /*
    fprintf(stderr, "gmel_memfunc_call name %s argc %d\n", func_name, argc);
    for(i=0; i<argc; ++i)
        fprintf(stderr, "\rarg %d: %s\n", i, argv[i]);
    */

    gmel_strview func_name_key_view = gmel_strview_from_zero_nocopy(func_name);

    gmel_memfunc* func_info =
        (gmel_memfunc*)ht_lookup(&GMEL_MEMFUNC_TABLE, &func_name_key_view);

    if (!func_info) gmel_error("no func_info for func %s", func_name);

    tmp_argc = func_info->argc + argc;

    if (func_info->binded_func_ptr) {
        /* argv[args] must be NULL, so argc + 1 */
        tmp_argv = (char**)GMEL_ALLOC((tmp_argc + 1) * sizeof(char*));

        memcpy((void*)tmp_argv, (void*)func_info->argv,
               func_info->argc * sizeof(char*));
        memcpy((void*)(tmp_argv + func_info->argc), (void*)argv,
               (argc + 1) * sizeof(char*));

        result = func_info->binded_func_ptr(func_name, tmp_argc, tmp_argv);

        GMEL_FREE(tmp_argv);
        return result;
    }

    /* poor, poor api. without binded_func_ptr there is no way except of
     * gmk_expand */
    Vector vector;
    vector_setup(&vector, 256, sizeof(char));
    vector_push_back_string(&vector, "$(call ", 0);
    vector_push_back_string(&vector, func_info->binded_func_name.data, 0);
    for (i = 0; i < func_info->argc; ++i) {
        vector_push_back_string(&vector, ",", 1);
        vector_push_back_string(&vector, func_info->argv[i], 0);
    }
    for (i = 0; i < argc; ++i) {
        vector_push_back_string(&vector, ",", 1);
        vector_push_back_string(&vector, argv[i], 0);
    }
    vector_push_back_string(&vector, ")", 1);
    vector_push_back_string(&vector, "\0", 1);

    result = gmk_expand((char*)vector_front(&vector));

    vector_destroy(&vector);
    return result;
}

char* gmel_bind_internal(char* target_func_name, char* bind_func_name, int argc,
                         char** argv) {
    /*
        target_func_name - new func name
        bind_func_name - existed func name
        argc/argv - extra args for call
    */
    char* err_str = "";
    size_t len;
    int i;

    gmel_memfunc func_info;
    func_info.binded_func_ptr = NULL;
    func_info.binded_func_name = gmel_strview_empty();

    if (!strcmp(target_func_name, bind_func_name))
        gmel_error("cannot bind '%s' to itself", target_func_name);

    /* check for existance */
    gmel_strview target_func_name_key_view =
        gmel_strview_from_zero_nocopy(target_func_name);

    if (ht_contains(&GMEL_MEMFUNC_TABLE, &target_func_name_key_view)) {
        err_str = safe_sprintf("$(error %s: function already exsists)",
                               target_func_name_key_view.data);
        gmk_expand(err_str);
        /* never reached */
        perror(err_str);
        abort();
        return NULL;
    }

    func_info.binded_func_name = gmel_strview_from_zero(bind_func_name);
    if (ht_contains(&GMEL_MEMFUNC_TABLE, &func_info.binded_func_name)) {
        func_info.binded_func_ptr =
            ((gmel_memfunc*)ht_lookup(&GMEL_MEMFUNC_TABLE,
                                      &func_info.binded_func_name))
                ->binded_func_ptr;
    }

    gmel_strview func_name_key = gmel_strview_from_zero(target_func_name);

    func_info.argc = argc;
    if (func_info.argc) {
        func_info.argv = (char**)GMEL_ALLOC(func_info.argc * sizeof(char*));
        for (i = 0; i < argc; ++i) {
            len = strlen(argv[i]) + 1;
            func_info.argv[i] = (char*)GMEL_ALLOC(len);
            memcpy(func_info.argv[i], argv[i], len);
        }
    } else {
        func_info.argv = NULL;
    }

    /* now GMEL_MEMFUNC_TABLE is owner of allocated data in func_name_key and
     * func_info */
    if (ht_insert(&GMEL_MEMFUNC_TABLE, &func_name_key, &func_info)) {
        gmk_expand("$(error exec_bind::ht_insert");
        abort();
        return NULL;
    }

    gmk_add_function(func_name_key.data, (gmk_func_ptr)gmel_memfunc_call, 0, 0,
                     GMK_FUNC_NOEXPAND);
    /*
    fprintf(stderr, "memorized func as '%s' for '%s' with argc %d,
    is_binded_func_ptr %d \n", func_name_key.data,
    func_info.binded_func_name.data, func_info.argc,
    (int)(func_info.binded_func_ptr != NULL)); for (i = 0; i < func_info.argc;
    ++i) { fprintf(stderr, "\targ %d: %s\n", i, func_info.argv[i]);
    }
    */
    return NULL;
}

char* gmel_bind(char* func_name, int argc, char** argv) {
    return gmel_bind_internal(argv[0], argv[1], argc - 2, argv + 2);
}
char* gmel_bind_r(char* func_name, int argc, char** argv) {
    return gmel_bind_internal(argv[1], argv[0], argc - 2, argv + 2);
}

int gmel_memfunc_table_setup() {
    GMEL_ASSERT(!ht_setup(&GMEL_MEMFUNC_TABLE, sizeof(gmel_strview),
                          sizeof(gmel_memfunc), 1024));

    GMEL_MEMFUNC_TABLE.compare = gmel_ht_strview_compare;
    GMEL_MEMFUNC_TABLE.hash = gmel_ht_strview_hash;
    GMEL_MEMFUNC_TABLE.key_free = (free_t)gmel_memfunc_tabkle_key_free;
    GMEL_MEMFUNC_TABLE.value_free = (free_t)gmel_memfunc_tabkle_value_free;

    return 1;
}
