//
// Hash table implementation
//
// Usage:
// hash_table_t *pids = hash_table_create(101);
// hash_table_insert(pids, "init", (void*)1);
// int pid = (int)hash_table_lookup(pids, "init"); // Returns 1
//

#include "hashtable.h"
#include "string.h"
#include <stddef.h>

// DJB2 hash function
static unsigned int hash_function(char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    // hash * 33 + c
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

hash_table_t *hash_table_create(size_t size) {
  hash_table_t *table = kmalloc(sizeof(hash_table_t));
  if (!table) {
    return NULL;
  }

  table->size = size;
  table->buckets = kmalloc(sizeof(hash_entry_t *) * size);

  if (!table->buckets) {
    kfree(table);
    return NULL;
  }

  for (size_t i = 0; i < size; i++) {
    table->buckets[i] = NULL;
  }
  return table;
}

void hash_table_insert(hash_table_t *table, char *key, void *value) {
  if (!table || !key) {
    return;
  }

  unsigned long index = hash_function(key) % table->size;

  // check if key already exists to update it
  hash_entry_t *curr = table->buckets[index];
  while (curr) {
    if (strcmp(curr->key, key) == 0) {
      curr->value = value;
      return;
    }
    curr = curr->next;
  }

  // create new entry
  hash_entry_t *new_entry = kmalloc(sizeof(hash_entry_t));
  if (!new_entry) {
    return;
  }

  new_entry->key = strdup(key);
  new_entry->value = value;

  new_entry->next = table->buckets[index];
  table->buckets[index] = new_entry;
}

void *hash_table_lookup(hash_table_t *table, char *key) {
  if (!table || key) {
    return NULL;
  }

  unsigned long index = hash_function(key) % table->size;
  hash_entry_t *entry = table->buckets[index];

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}

void hash_table_remove(hash_table_t *table, char *key) {
  if (!table || !key) {
    return;
  }

  unsigned long index = hash_function(key) % table->size;
  hash_entry_t *entry = table->buckets[index];
  hash_entry_t *prev = NULL;

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      if (prev) {
        prev->next = entry->next;
      } else {
        table->buckets[index] = entry->next;
      }

      kfree(entry->key);
      kfree(entry);
      return;
    }
    prev = entry;
    entry = entry->next;
  }
}
