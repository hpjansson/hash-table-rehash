#include "khash-impl.h"

KHASH_MAP_INIT_INT64(kh64, void *)
KHASH_MAP_INIT_STR(khstr, void *)

#define SETUP \
  khash_t(kh64) *hash = kh_init(kh64); \
  khash_t(khstr) *str_hash = kh_init(khstr);

#define INSERT_INT_INTO_HASH(key, value) \
  do { int r; khint_t k = kh_put (kh64, hash, key, &r); if (r > 0) { kh_value (hash, k) = &value; } } while (0)
#define DELETE_INT_FROM_HASH(key) \
  do { khint_t k = kh_get (kh64, hash, key); \
       if (k != kh_end (hash)) kh_del (kh64, hash, k); } while (0)
#define INSERT_STR_INTO_HASH(key, value) \
  do { int r; char *istr = strdup (key); khint_t k = kh_put (khstr, str_hash, istr, &r); if (r > 0) { kh_value (str_hash, k) = &value; } else free (istr); } while (0)
#define DELETE_STR_FROM_HASH(key) \
  do { khint_t k = kh_get (khstr, str_hash, key); \
       if (k != kh_end (hash)) { char *istr = (char *) kh_key (str_hash, k); kh_del (khstr, str_hash, k); free (istr); } } while (0)
#define LOOKUP_INT(key) \
    do { khint_t k = kh_get (kh64, hash, key);                          \
        if (k != kh_end (hash)) { volatile void *v = kh_value (hash, k); } } while (0);

#include "template.c"
