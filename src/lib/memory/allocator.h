#ifndef _MEMORY_ALLOCATOR_H_
#define _MEMORY_ALLOCATOR_H_

#include <stdlib.h>

// typedef struct r_mem_allocator {
//     void * (*alloc)(size_t size);
//     void * (*realloc)(void *ptr, size_t size);
//     void   (*free)(void *ptr);
//     char * tag;
// } r_mem_allocator;

#define MEMORY_DEBUG 1

#define MALLOC(type, size) r_malloc(#type, sizeof(type) * size);

#define FREE(type, ptr) \
    r_free(#type, ptr); \
    ptr = NULL;

void * r_malloc(const char *type, size_t size);
void   r_free(const char *type, void *ptr);

#endif