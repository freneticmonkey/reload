#ifndef SOX_ITERATOR_H
#define SOX_ITERATOR_H

#include "value.h"

// container type
typedef void container;

// iterator next type enum
typedef enum iterator_next_type_t {
    ITERATOR_NEXT_TYPE_NONE,
    ITERATOR_NEXT_TYPE_INDEX,
    ITERATOR_NEXT_TYPE_KEY,
} iterator_next_type_t;

typedef struct iterator_next_t {
    iterator_next_type_t type;
    value_t *value;
    int      index;
    int      next_index;
    obj_string_t *key;
} iterator_next_t;

iterator_next_t l_iterator_next_null();

// container functions
typedef iterator_next_t (*item_next)(container *data, iterator_next_t last);
typedef int             (*item_count)(container *data);

// iterator type
typedef struct iterator_t {
    // wrapped container
    container *data;

    // iterator's current value
    iterator_next_t current;

    // container's next item function pointer 
    item_next next;

    // container's item count function pointer
    item_count count;
} iterator_t;

// iterator lifecycle
void l_init_iterator(iterator_t *it, container *data, item_next next, item_count count);
void l_free_iterator(iterator_t *it);

// iterator helpers
iterator_next_t l_iterator_next(iterator_t *it);
int             l_iterator_index(iterator_t *it);
value_t *       l_iterator_value(iterator_t *it);
int             l_iterator_count(iterator_t *it);

bool            l_iterator_has_next(iterator_next_t next);

#endif