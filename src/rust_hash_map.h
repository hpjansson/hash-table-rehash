#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct IntHash IntHash;

typedef struct StrHash StrHash;

void delete_int_from_hash(IntHash *hash, int64_t key);

void delete_str_from_hash(StrHash *hash, const char *key);

void insert_int_into_hash(IntHash *hash, int64_t key, int64_t value);

void insert_str_into_hash(StrHash *hash, const char *key, int64_t value);

const int64_t *lookup_int(const IntHash *hash, int64_t key);

IntHash *new_int_hash(void);

StrHash *new_str_hash(void);
