#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../memory/memory.h"
#include <stddef.h>

typedef struct hash_entry {
  char *key;
  void *value;
  struct hash_entry *next;
} hash_entry_t;

typedef struct {
  size_t size;
  hash_entry_t **buckets;
} hash_table_t;

hash_table_t *hash_table_create(size_t size);
void hash_table_insert(hash_table_t *table, char *key, void *value);
void *hash_table_lookup(hash_table_t *table, char *key);
void hash_table_remove(hash_table_t *table, char *key);

#endif
