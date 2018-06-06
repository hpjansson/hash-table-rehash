#include "khash-impl.h"

KHASH_MAP_INIT_INT64(kh64, void *)
KHASH_MAP_INIT_STR(khstr, void *)

#define SETUP \
  khash_t(kh64) *hash = kh_init(kh64); \
  khash_t(khstr) *str_hash = kh_init(khstr);

#define INSERT_INT_INTO_HASH(key, value) kh_put (kh64, hash, key, &value)
#define DELETE_INT_FROM_HASH(key) \
  do { khint_t k = kh_get (kh64, hash, key); \
       kh_del (kh64, hash, k); } while (0)
#define INSERT_STR_INTO_HASH(key, value) kh_put (khstr, str_hash, key, &value)
#define DELETE_STR_FROM_HASH(key) \
  do { khint_t k = kh_get (khstr, str_hash, key); \
       kh_del (khstr, str_hash, k); } while (0)

#include "template.c"
