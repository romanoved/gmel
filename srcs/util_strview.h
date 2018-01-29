#pragma once

#include <stddef.h>

typedef struct gmel_strview {
    char const* data;
    size_t size;
} gmel_strview;

void gmel_strview_free(gmel_strview* ptr);
gmel_strview gmel_strview_empty(void);
gmel_strview gmel_strview_from_zero_nocopy(char const* data);
gmel_strview gmel_strview_from_zero(char const* data);
int gmel_strview_cmp(const void* p1, const void* p2);
int gmel_ht_strview_compare(void* first_key, void* second_key, size_t key_size);
size_t gmel_ht_strview_hash(void* raw_key, size_t key_size);
