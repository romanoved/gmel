#include "util_strview.h"
#include "util_common.h"

#include "util_hashtable.h"

#include <string.h>

void gmel_strview_free(gmel_strview* ptr) {
    GMEL_FREE(ptr->data);
    ptr->data = NULL;
    ptr->size = 0;
}

gmel_strview gmel_strview_empty() {
    gmel_strview result;
    result.size = 0;
    result.data = NULL;
    return result;
}
gmel_strview gmel_strview_from_zero_nocopy(char const* data) {
    gmel_strview result;
    result.size = strlen(data) + 1;
    result.data = data;
    return result;
}

gmel_strview gmel_strview_from_zero(char const* data) {
    gmel_strview result;
    result.size = strlen(data) + 1;
    result.data = (char const*)GMEL_ALLOC(result.size);
    memcpy((char*)result.data, data, result.size);
    return result;
}

int gmel_strview_cmp(const void* p1, const void* p2) {
    gmel_strview* sv1 = (gmel_strview*)p1;
    gmel_strview* sv2 = (gmel_strview*)p2;
    unsigned long c_size = sv1->size < sv2->size ? sv1->size : sv2->size;
    int c_result = memcmp(sv1->data, sv2->data, c_size);
    return c_result
               ? c_result
               : (sv1->size < sv2->size ? -1 : (sv1->size > sv2->size ? 1 : 0));
}

int gmel_ht_strview_compare(void* first_key, void* second_key,
                            size_t key_size) {
    return gmel_strview_cmp(first_key, second_key);
}

size_t gmel_ht_strview_hash(void* raw_key, size_t key_size) {
    gmel_strview* key = (gmel_strview*)raw_key;
    return _ht_default_hash((void*)key->data, key->size);
}
