#ifndef _MEMORY_ALLOCATOR_H_
#define _MEMORY_ALLOCATOR_H_

#include <stdlib.h>

// typedef struct r_mem_allocator {
//     void * (*alloc)(size_t size);
//     void * (*realloc)(void *ptr, size_t size);
//     void   (*free)(void *ptr);
//     char * tag;
// } r_mem_allocator;

#define MALLOC(type, size) \
    malloc(sizeof(type) * size);

#define FREE(type, ptr) \
    free(ptr); \
    ptr = NULL;

#endif