#include <glib.h>
#define SETUP GHashTable * hash     = g_hash_table_new(g_direct_hash, g_direct_equal); \
              GHashTable * str_hash = g_hash_table_new_full(g_str_hash,    g_str_equal, free, NULL);
#define INSERT_INT_INTO_HASH(key, value) g_hash_table_insert(hash, GINT_TO_POINTER(key), &value)
#define DELETE_INT_FROM_HASH(key) g_hash_table_remove(hash, GINT_TO_POINTER(key))
#define INSERT_STR_INTO_HASH(key, value) g_hash_table_insert(str_hash, strdup (key), &value)
#define DELETE_STR_FROM_HASH(key) g_hash_table_remove(str_hash, key)
#define LOOKUP_INT(key) \
    do { volatile gpointer v = g_hash_table_lookup (hash, GINT_TO_POINTER (key)); } while (0);

#include "template.c"
