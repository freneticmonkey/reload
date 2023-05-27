#ifndef SOX_LIST_H
#define SOX_LIST_H

#include "lib/memory.h"

typedef struct obj_list_t {
    obj_t obj;
    int length;
    value_t *values;
} obj_list_t;

obj_list_t* l_new_list();
obj_list_t* l_new_list_from_list(obj_list_t *from);

void l_print_list(obj_list_t *list);

#endif