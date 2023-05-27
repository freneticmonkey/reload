#ifndef SOX_TABLE_H
#define SOX_TABLE_H

#include "common.h"
#include "value.h"
#include "iterator.h"

typedef struct {
    obj_string_t* key;
    value_t value;
} entry_t;

typedef struct {
  int count;
  int capacity;
  entry_t* entries;
} table_t;

void l_init_table(table_t *table);
void l_free_table(table_t* table);

bool l_table_get(table_t* table, obj_string_t* key, value_t* value);

bool l_table_set(table_t* table, obj_string_t* key, value_t value);
bool l_table_delete(table_t* table, obj_string_t* key);
void l_table_add_all(table_t* from, table_t* to);

// iterator functions
void l_table_get_iterator(table_t * table, iterator_t * it);

obj_string_t* l_table_find_string(table_t* table, const char* chars, size_t length, uint32_t hash);

// deserialisation helper
entry_t * l_table_set_entry(table_t * table, obj_string_t * key);

// garbage collection
void l_mark_table(table_t* table);
void l_table_remove_white(table_t* table);

#endif