#include <stdio.h>

#include "memory/allocator.h"

void * r_malloc(const char *type, size_t size) {
    void *ptr = malloc(size);
#ifdef MEMORY_DEBUG
    printf("malloc(%s, %zu) = %p\n", type, size, ptr);
#endif
    return ptr;
}

void r_free(const char *type, void *ptr) {
#ifdef MEMORY_DEBUG
    printf("type(%s): free(%p)\n", type, ptr);
#endif
    free(ptr);
    ptr = NULL;
}