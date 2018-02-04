#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "bind.h"

#include "util_common.h"
#include "util_hashtable.h"
#include "util_strview.h"
#include "util_vector.h"


/* dynamically bind popen wrapper */

static HashTable GMEL_MEMFUNC_TABLE;

typedef struct gmel_memfunc {
    int argc;
    char** argv;
    int should_expand;
    gmk_func_ptr binded_func_ptr;
    gmel_strview binded_func_name;
} gmel_memfunc;

void gmel_print_mft_value(gmel_memfunc* value) {
    int i;
    if (!value) {
        fprintf(stderr, "gmel_memfunc is NULL\n");
        return;
    }
    fprintf(stderr, "\tfunc_name: '%s'\n", value->binded_func_name.data);
    fprintf(stderr, "\tfunc_ptr: %p\n", value->binded_func_ptr);
    fprintf(stderr, "\tshould_expand: %d\n", value->should_expand);
    fprintf(stderr, "\targc: %d\n", value->argc);
    for (i = 0; i < value->argc; ++i)
        fprintf(stderr, "\t\targv[%d]: '%s'\n", i, value->argv[i]);
}

char* gmel_print_mft(char* func_name, int argc, char** argv) {
    HTNode* node;
    size_t chain;

    fprintf(stderr, "\nGMEL_MEMFUNC_TABLE: mft at %p:\n",
            (void*)&GMEL_MEMFUNC_TABLE);
    for (chain = 0; chain < GMEL_MEMFUNC_TABLE.capacity; ++chain) {
        for (node = GMEL_MEMFUNC_TABLE.nodes[chain]; node; node = node->next) {
            fprintf(stderr, "key: '%s'\n", ((gmel_strview*)node->key)->data);
            gmel_print_mft_value((gmel_memfunc*)node->value);
        }
    }
    fprintf(stderr, "GMEL_MEMFUNC_TABLE: done\n\n");
    return NULL;
}

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

    gmel_strview func_name_key_view;
    gmel_memfunc* func_info = NULL;
    gmel_memfunc* next_func_info = NULL;

    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr, "gmel_memfunc_call name %s argc %d\n", func_name, argc);
        for (i = 0; i < argc; ++i)
            fprintf(stderr, "\targv[%d]: '%s'\n", i, argv[i]);
    }

    Vector extra_args;
    vector_setup(&extra_args, 32, sizeof(char*));
    for (i = argc - 1; i >= 0; --i) {
        GMEL_ASSERT(!vector_push_front(&extra_args, &argv[i]));
    }
    func_name_key_view = gmel_strview_from_zero_nocopy(func_name);
    while (1) {
        if (GMEL_TRACE_LEVEL) {
            fprintf(stderr, "gmel_memfunc_call: lookup '%s'\n",
                    func_name_key_view.data);
        }
        next_func_info =
            (gmel_memfunc*)ht_lookup(&GMEL_MEMFUNC_TABLE, &func_name_key_view);
        if (!next_func_info) {
            if (GMEL_TRACE_LEVEL) {
                fprintf(stderr,
                        "gmel_memfunc_call: no record in GMEL_MEMFUNC_TABLE\n");
            }
            break;
        }
        func_info = next_func_info;
        if (GMEL_TRACE_LEVEL) {
            fprintf(stderr, "gmel_memfunc_call: got data:\n");
            gmel_print_mft_value(func_info);
        }
        for (i = func_info->argc - 1; i >= 0; --i) {
            GMEL_ASSERT(!vector_push_front(&extra_args, &func_info->argv[i]));
        }
        if (!gmel_strview_cmp(&func_name_key_view,
                              &func_info->binded_func_name)) {
            if (GMEL_TRACE_LEVEL) {
                fprintf(stderr,
                        "gmel_memfunc_call: key == binded_func_name, stop "
                        "lookup\n");
            }
            break;
        }
        func_name_key_view = func_info->binded_func_name;
    }

    if (!func_info) gmel_error("no func_info for func %s", func_name);

    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr, "last bind name '%s', func_info:\n",
                func_name_key_view.data);
        gmel_print_mft_value(func_info);
        fprintf(stderr, "extra_argv vector:\n");
        for (i = 0; i < extra_args.size; ++i)
            fprintf(stderr, "\textra_argv[%d]: '%s'\n", i,
                    *(const char**)vector_const_get(&extra_args, i));
    }

    if (!func_info->should_expand) {  // TODO: if by func_info->binded_func_ptr
        GMEL_ASSERT(func_info->binded_func_ptr);
        tmp_argv = (char**)GMEL_ALLOC((extra_args.size + 1) * sizeof(char*));
        for (i = 0; i < extra_args.size; ++i)
            tmp_argv[i] = *(char**)vector_const_get(&extra_args, i);
        tmp_argv[extra_args.size] = NULL;
        tmp_argc = extra_args.size;
        result = func_info->binded_func_ptr(func_name, tmp_argc, tmp_argv);
        GMEL_FREE(tmp_argv);
        return result;
    }

    Vector vector;
    vector_setup(&vector, 256, sizeof(char));
    vector_push_back_string(&vector, "$(call ", 0);
    vector_push_back_string(&vector, func_name_key_view.data, 0);
    for (i = 0; i < extra_args.size; ++i) {
        vector_push_back_string(&vector, ",", 1);
        vector_push_back_string(
            &vector, *(const char**)vector_const_get(&extra_args, i), 0);
    }
    vector_push_back_string(&vector, ")", 1);
    vector_push_back_string(&vector, "\0", 1);
    vector_destroy(&extra_args);
    if (GMEL_TRACE_LEVEL)
        fprintf(stderr, "EXPANDED: %s\n", (char*)vector_front(&vector));
    result = gmk_expand((char*)vector_front(&vector));
    if (GMEL_TRACE_LEVEL) fprintf(stderr, "EXPAND RUN DONE\n");
    vector_destroy(&vector);
    return result;
}

char* gmel_bind_internal(unsigned int min_args, unsigned int max_args,
                         char* target_func_name, char* bind_func_name, int argc,
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
    func_info.should_expand = 1;
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

    gmk_add_function(func_name_key.data, (gmk_func_ptr)gmel_memfunc_call,
                     min_args, max_args, GMK_FUNC_NOEXPAND);
    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr,
                "memorized func as '%s' for '%s' with argc %d, "
                "binded_func_ptr %p \n",
                func_name_key.data, func_info.binded_func_name.data,
                func_info.argc, (void*)func_info.binded_func_ptr);
        for (i = 0; i < func_info.argc; ++i) {
            fprintf(stderr, "\targ %d: '%s'\n", i, func_info.argv[i]);
        }
    }
    return NULL;
}

char* gmel_bind(char* func_name, int argc, char** argv) {
    return gmel_bind_internal(0, 0, argv[0], argv[1], argc - 2, argv + 2);
}

char* gmel_bind_r(char* func_name, int argc, char** argv) {
    return gmel_bind_internal(0, 0, argv[1], argv[0], argc - 2, argv + 2);
}

char* gmel_bind_args(char* func_name, int argc, char** argv) {
    return gmel_bind_internal(parse_uint(argv[0]), parse_uint(argv[1]), argv[2],
                              argv[3], argc - 4, argv + 4);
}

char* gmel_mft_register(char* func_name, int argc, char** argv) {
    const char* bfunc_name = (const char*)argv[0];
    const char* symb_name = (const char*)argv[1];
    int should_expand = parse_uint(argv[2]);
    void* st_handler = NULL;
    void* func_ptr = NULL;

    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr, "gmel_mft_register for %s with symbol %s\n", bfunc_name,
                symb_name);
    }
    GMEL_ASSERT(st_handler = dlopen(NULL, RTLD_LAZY));
    GMEL_ASSERT(func_ptr = dlsym(st_handler, symb_name));

    if (GMEL_TRACE_LEVEL) {
        fprintf(stderr, "gmel_mft_register: found symbol at %p\n",
                (void*)func_ptr);
    }

    gmel_memfunc func_info;
    gmel_strview func_name_key;
    func_name_key = gmel_strview_from_zero(bfunc_name);
    func_info.binded_func_name = gmel_strview_from_zero(bfunc_name);
    func_info.binded_func_ptr = (gmk_func_ptr)func_ptr;
    func_info.argc = 0;
    func_info.argv = NULL;
    func_info.should_expand = should_expand;
    GMEL_ASSERT(!ht_insert(&GMEL_MEMFUNC_TABLE, &func_name_key, &func_info));
    return NULL;
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
