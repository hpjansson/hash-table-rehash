#include "rust_hash_map.h"
#define SETUP IntHash * hash     = new_int_hash(); \
              StrHash * str_hash = new_str_hash();
#define INSERT_INT_INTO_HASH(key, value) insert_int_into_hash(hash, key, value)
#define DELETE_INT_FROM_HASH(key) delete_int_from_hash(hash, key)
#define INSERT_STR_INTO_HASH(key, value) insert_str_into_hash(str_hash, key, value)
#define DELETE_STR_FROM_HASH(key) delete_str_from_hash(str_hash, key)
#define LOOKUP_INT(key) \
    do { volatile const int64_t * v = lookup_int(hash, key); } while (0);

#include "template.c"
