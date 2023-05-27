#include "lib/iterator.h"
#include "lib/memory.h"

iterator_next_t l_iterator_next_null() {
    return (iterator_next_t) { 
        .type = ITERATOR_NEXT_TYPE_NONE,
        .value = NULL,
        .key = NULL,
        .index = 0,
        .next_index = 0,
    };
}

// new iterator
void l_init_iterator(iterator_t *it, container *data, item_next next, item_count count) {
    it->data = data;
    it->current = (iterator_next_t) { 
        .type = ITERATOR_NEXT_TYPE_NONE,
        .value = NULL,
        .key = NULL,
        .index = 0,
        .next_index = 0,
    };
    it->next = next;
    it->count = count;
    // run the first iteration
    l_iterator_next(it);
}

void l_free_iterator(iterator_t *it) {
    FREE(iterator_t, it);
}

iterator_next_t l_iterator_next(iterator_t *it) {
    it->current = it->next(it->data, it->current);
    return it->current;
}

int l_iterator_index(iterator_t *it) {
    return it->current.index;
}

value_t * l_iterator_value(iterator_t *it) {
    return it->current.value;
}

int l_iterator_count(iterator_t *it) {
    return it->count(it->data);
}

bool l_iterator_has_next(iterator_next_t next) {
    return next.type != ITERATOR_NEXT_TYPE_NONE;
}