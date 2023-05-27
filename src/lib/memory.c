#include <stdlib.h>
#include <string.h>

#include "lib/memory.h"
// #include "vm.h"

// allow separately controlled debug allocation tracking
#if defined(DEBUG_LOG_ALLOC)
    #define DEBUG_ALLOCATIONS 1
#endif

#if defined(DEBUG_LOG_GC)
#include <stdio.h>
#include "debug.h"
#endif

#if defined(DEBUG_LOG_ALLOC)

#if defined(DEBUG_ALLOCATIONS)
typedef struct alloc_t alloc_t;
typedef struct alloc_t {
    void       *address;
    size_t      size;
    const char *file;
    int         line;
    alloc_t    *next;
    bool        freed;
} alloc_t;

// Create a new allocation record
static alloc_t *_alloc_record(alloc_t *head, size_t size, void *address, const char *file, int line) {
    alloc_t *alloc = malloc(sizeof(alloc_t));
    alloc->size = size;
    alloc->address = address;
    alloc->file = file;
    alloc->line = line;
    alloc->next = head;
    alloc->freed = false;
    
    return alloc;
}

// clean up allocation records linked list
static void _alloc_free_records(alloc_t *head) {
    alloc_t *alloc = head;
    while (alloc != NULL) {
        alloc_t *next = alloc->next;
        free(alloc);
        alloc = next;
    }
}

// mark allocation record as freed
static alloc_t *_alloc_mark_freed(alloc_t *head, void *address) {
    alloc_t *alloc = head;
    while (alloc != NULL) {
        if (alloc->address == address) {
            // if already freed, display error
            if (alloc->freed) {
                printf("ERROR: Double free of %p in %s:%d\n", alloc->address, alloc->file, alloc->line);
                return head;
            }
            alloc->freed = true;
            return alloc;
        }
        alloc = alloc->next;
    }
    return NULL;
}

// clean up specific allocation record
static alloc_t *_alloc_free(alloc_t *head, void *address) {
    alloc_t *alloc = head;
    alloc_t *prev = NULL;
    while (alloc != NULL) {
        if (alloc->address == address) {
            if (prev == NULL) {
                head = alloc->next;
            } else {
                prev->next = alloc->next;
            }
            free(alloc);
            return head;
        }
        prev = alloc;
        alloc = alloc->next;
    }
    return head;
}

// find allocation for size
static alloc_t *_alloc_find_size(alloc_t *head, size_t size) {
    alloc_t *alloc = head;
    while (alloc != NULL) {
        if (alloc->size == size) {
            return alloc;
        }
        alloc = alloc->next;
    }
    return NULL;
}

// print allocation record
static void _alloc_print(alloc_t *alloc) {
    printf("Allocated %zu bytes at %p in %s:%d\n", alloc->size, alloc->address, alloc->file, alloc->line);
}

// print allocation records linked list
static void _alloc_print_records(alloc_t *head) {
    alloc_t *alloc = head;
    printf("Allocation Tracking\n");
    printf("-------------------\n");
    while (alloc != NULL) {
        _alloc_print(alloc);
        alloc = alloc->next;
    }
    printf("-------------------\n");
}

// find allocation record for address
static alloc_t *_alloc_find(alloc_t *head, void *address) {
    alloc_t *alloc = head;
    while (alloc != NULL) {
        if (alloc->address == address) {
            return alloc;
        }
        alloc = alloc->next;
    }
    return NULL;
}
#endif
#endif

#define GC_HEAP_GROW_FACTOR 2

static size_t _internal_alloc = 0;
static size_t _internal_dealloc = 0;
static size_t _internal_vm_alloc_max = 0;


// native_lookup_t
// the following structure tracks a const char * native function name to a obj_native_t pointer
// this is used to resolve the native function pointer when a function is deserialised
typedef struct {
    const char *name;
    obj_native_t *native;
} native_lookup_item_t;

typedef struct native_lookup_t {
    native_lookup_item_t *items;
    size_t count;
    size_t capacity;
} native_lookup_t;

// object_lookup_t
// the following structure maps uintptr_t ids to obj_t pointers and stores an array of pointers
// which are resolved at the end of deserialisation

typedef struct {
    uintptr_t id;
    size_t count;
    size_t capacity;
    obj_t *address;
    obj_t * **registered_targets;
} object_lookup_item_t;

typedef struct object_lookup_t {
    size_t count;
    size_t capacity;
    object_lookup_item_t *items;
} object_lookup_t;

typedef struct string_lookup_item_t {
    uint32_t hash;
    size_t count;
    size_t capacity;
    obj_string_t *address;
    obj_string_t * **registered_targets;
} string_lookup_item_t;

typedef struct string_lookup_t {
    size_t count;
    size_t capacity;
    string_lookup_item_t *items;
} string_lookup_t;

typedef struct mem_track {
    // memory tracking
    bool track_allocations;
    size_t bytes_allocated;
    native_lookup_t native_lookup;
    object_lookup_t object_lookup;
    string_lookup_t string_lookup;
#if defined(DEBUG_ALLOCATIONS)
    alloc_t *allocations;
#endif
} mem_track_t;

typedef struct {
    // garbage collection
    size_t bytes_allocated;
    size_t next_gc;
    int    gray_count;
    int    gray_capacity;
    obj_t** gray_stack;
    obj_t* objects;

    int          roots_cb_count;
    int          roots_cb_capacity;
    l_mark_roots_callback *roots_cb;

    int gc_cb_count;
    int gc_cb_capacity;
    l_gc_callback *gc_cb;

    // memory tracking
    mem_track_t track;    
} mem_t;

static mem_t _mem;

size_t l_calculate_capacity_with_size(size_t current_capacity, size_t new_size) {
    size_t new_capacity = current_capacity;
    while (new_capacity < new_size) {
        new_capacity = GROW_CAPACITY(new_capacity);
        // check if new_capacity is unreasonably large
        if (new_capacity > SIZE_MAX) {
            printf("new_capacity is unreasonably large: %zu trying to expand to accommodate: %zu\n", new_capacity, new_size);
            exit(1);
        }
    }
    return new_capacity;
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize, const char * filename, int line) {

    // deallocating
    size_t alloc_size = newSize - oldSize;
#if defined(DEBUG_LOG_GC)
    size_t vm_bytes = _mem.bytes_allocated;
#endif
    size_t dealloc_readable = (SIZE_MAX - alloc_size + 1);
    if ( (newSize < oldSize) && ( dealloc_readable > _mem.bytes_allocated ) ) {
        
        printf("detected untracked vm memory. vm alloc bytes: %zu. dealloc bytes: %zu location: (%s:%d)\n", 
                _mem.bytes_allocated, 
                dealloc_readable,
                filename,
                line
        );
        printf("internal tracking. alloc bytes: %zu. dealloc bytes: %zu vm max alloc: %zu\n", 
                _internal_alloc, 
                _internal_dealloc,
                _internal_vm_alloc_max
        );

        printf("internal tracking. %p %s: %zu alloc bytes: %zu. dealloc bytes: %zu vm alloc: %zu\n", 
                pointer,
                (newSize > oldSize) ? "ALLOC" : "DEALLOC",
                (newSize > oldSize) ? alloc_size : dealloc_readable,
                _internal_alloc, 
                _internal_dealloc,
                _mem.bytes_allocated
            );
        
        exit(1);
    }

    if (newSize > oldSize) {
        _internal_alloc += alloc_size;
    } else {
        _internal_dealloc += dealloc_readable;
    }   

    if ( _mem.bytes_allocated > _internal_vm_alloc_max ) {
        _internal_vm_alloc_max = _mem.bytes_allocated;
    }

    _mem.bytes_allocated += alloc_size;

    if (newSize > oldSize) {
#if defined(DEBUG_STRESS_GC)
        l_collect_garbage();
#endif
    }

    if (_mem.bytes_allocated > _mem.next_gc) {
        l_collect_garbage();
    }

#if defined(DEBUG_LOG_GC)
    if (newSize != 0 && oldSize != 0) {
        printf("internal tracking. %p %s: %zu alloc bytes: %zu. dealloc bytes: %zu vm alloc: %zu\n", 
                pointer,
                "REALLOC SIZE CHANGE <<",
                dealloc_readable,
                _internal_alloc, 
                _internal_dealloc,
                _mem.bytes_allocated
            );
    }
#endif

    if (newSize == 0) {

#if defined(DEBUG_LOG_GC)
    if (pointer == NULL) {
        printf("[free] <(nil)            vm bytes: %zu->%zu.\t dealloc bytes:-%zu\n",
                vm_bytes,
                _mem.bytes_allocated,
                (SIZE_MAX - alloc_size + 1)
        );

    } else {
        printf("[free] <%p\t vm bytes: %zu->%zu.\t dealloc bytes:-%zu\n",
                pointer,
                vm_bytes,
                _mem.bytes_allocated,
                (SIZE_MAX - alloc_size + 1)
        );
    }
    
#endif

#if defined(DEBUG_LOG_ALLOC)

#if defined(DEBUG_ALLOCATIONS)
    alloc_t *alloc = _alloc_find(_mem.track.allocations, pointer);

    if ( alloc != NULL ) {
        printf("internal tracking. %p %s: %zu alloc bytes: %zu. dealloc bytes: %zu vm alloc: %zu original location: (%s:%d)\n", 
                    pointer,
                    "DEALLOC <<",
                    dealloc_readable,
                    _internal_alloc, 
                    _internal_dealloc,
                    _mem.bytes_allocated,
                    alloc->file,
                    alloc->line
                );
    } else {
#endif
        printf("internal tracking. %p %s: %zu alloc bytes: %zu. dealloc bytes: %zu vm alloc: %zu location: (%s:%d)\n", 
                    pointer,
                    "DEALLOC <<",
                    dealloc_readable,
                    _internal_alloc, 
                    _internal_dealloc,
                    _mem.bytes_allocated,
                    filename,
                    line
                );
    
#if defined(DEBUG_ALLOCATIONS)
    }

    
    // record the deallocation into the tracking system
    _alloc_mark_freed(_mem.track.allocations, pointer);
#endif

#endif

        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

#if defined(DEBUG_LOG_ALLOC)
    printf("internal tracking. %p %s: %zu alloc bytes: %zu. dealloc bytes: %zu vm alloc: %zu location: (%s:%d)\n", 
                result,
                "  ALLOC >>",
                alloc_size,
                _internal_alloc, 
                _internal_dealloc,
                _mem.bytes_allocated,
                filename,
                line
            );
#if defined(DEBUG_ALLOCATIONS)
    // record the allocation into the tracking system
    alloc_t * alloc = _alloc_record(_mem.track.allocations, newSize, result, filename, line );

    if (alloc != NULL) {
        _mem.track.allocations = alloc;
    }
#endif
#endif

    if (result == NULL) {
        exit(1);
    }

#if defined(DEBUG_LOG_GC)
    printf("[new]  >%p\t vm bytes: %zu->%zu.\t alloc bytes: %zu\n",
            result,
            vm_bytes,
            _mem.bytes_allocated, 
            alloc_size
    );
#endif

    return result;
}

// initialise the memory management system
void l_init_memory() {

    _mem = (mem_t) {
        .bytes_allocated = 0,
        .next_gc = 1024 * 1024,

        .gray_count = 0,
        .gray_capacity = 0,
        .gray_stack = NULL,
        
        .objects = NULL,
        
        .roots_cb_count = 0,
        .roots_cb_capacity = 0,
        .roots_cb = NULL,
        
        .gc_cb_count = 0,
        .gc_cb_capacity = 0,
        .gc_cb = NULL,
        
        .track = (mem_track_t){
            .track_allocations = false,
            .bytes_allocated = 0,
            .native_lookup = {
                .items = NULL,
                .count = 0,
                .capacity = 0
            },
            .object_lookup = {
                .items = NULL,
                .count = 0,
                .capacity = 0
            }
#if defined(DEBUG_ALLOCATIONS)
            , .allocations = NULL,
#endif
        },
    };

    // // setup mark roots callback values
    _mem.roots_cb_capacity = 8;
    _mem.roots_cb = (l_mark_roots_callback*)malloc(sizeof(l_mark_roots_callback) * _mem.roots_cb_capacity);
    
    // setup the garbage collect callback values
    _mem.gc_cb_capacity = 8;
    _mem.gc_cb = (l_gc_callback*)malloc(sizeof(l_gc_callback) * _mem.gc_cb_capacity);
}
// cleanup the memory management system
void _free_objects();
void _free_object();

void l_free_memory() {

    // free all internal memory
    free(_mem.gray_stack);
    free(_mem.gc_cb);
    free(_mem.roots_cb);

    _free_objects();

#if defined(DEBUG_LOG_ALLOC)

#if defined(DEBUG_ALLOCATIONS)
    if (_mem.track.allocations != NULL) {
        _alloc_free_records(_mem.track.allocations);
    }
#endif

#endif

    _mem = (mem_t) {
        .bytes_allocated = 0,
        .next_gc = 1024 * 1024,

        .gray_count = 0,
        .gray_capacity = 0,
        .gray_stack = NULL,
        
        .objects = NULL,
        
        .roots_cb_count = 0,
        .roots_cb_capacity = 0,
        .roots_cb = NULL,
        
        .gc_cb_count = 0,
        .gc_cb_capacity = 0,
        .gc_cb = NULL,
        
        .track = (mem_track_t){
            .track_allocations = false,
            .bytes_allocated = 0,
            .native_lookup = {
                .items = NULL,
                .count = 0,
                .capacity = 0
            },
            .object_lookup = {
                .items = NULL,
                .count = 0,
                .capacity = 0
            },
            .string_lookup = {
                .items = NULL,
                .count = 0,
                .capacity = 0
            }
#if defined(DEBUG_ALLOCATIONS)
            , .allocations = NULL,
#endif
        },
    };

}

// garbage collection
void l_mark_object(obj_t* object) {
    if (object == NULL) 
        return;

    if (object->is_marked) 
        return;
    
#if defined(DEBUG_LOG_GC)
    printf("%p mark ", (void*)object);
    l_print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->is_marked = true;

    if (_mem.gray_capacity < _mem.gray_count + 1) {
        _mem.gray_capacity = GROW_CAPACITY(_mem.gray_capacity);
        _mem.gray_stack = (obj_t**)realloc(_mem.gray_stack, sizeof(obj_t*) * _mem.gray_capacity);

        if (_mem.gray_stack == NULL) 
            exit(1);
    }

    _mem.gray_stack[_mem.gray_count++] = object;
}

void l_mark_value(value_t value) {
  if (IS_OBJ(value))
    l_mark_object(AS_OBJ(value));
}

static void l_mark_array(value_array_t* array) {
    for (int i = 0; i < array->count; i++) {
        l_mark_value(array->values[i]);
    }
}

static void _blacken_object(obj_t* object) {
    if (object == NULL) {
        return;
    }

#if defined(DEBUG_LOG_GC)
    printf("%p blacken ", (void*)object);
    l_print_value(OBJ_VAL(object));
    printf("\n");
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD: {
            obj_bound_method_t* bound = (obj_bound_method_t*)object;
            l_mark_value(bound->receiver);
            l_mark_object((obj_t*)bound->method);
            break;
        }
        case OBJ_CLASS: {
            obj_class_t* klass = (obj_class_t*)object;
            l_mark_object((obj_t*)klass->name);
            l_mark_table(&klass->methods);
            break;
        }
        case OBJ_CLOSURE: {
            obj_closure_t* closure = (obj_closure_t*)object;
            l_mark_object((obj_t*)closure->function);
            for (int i = 0; i < closure->upvalue_count; i++) {
                l_mark_object((obj_t*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            obj_function_t* function = (obj_function_t*)object;
            l_mark_object((obj_t*)function->name);
            l_mark_array(&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE: {
            obj_instance_t* instance = (obj_instance_t*)object;
            l_mark_object((obj_t*)instance->klass);
            l_mark_table(&instance->fields);
            break;
        }
        case OBJ_UPVALUE:
            l_mark_value(((obj_upvalue_t*)object)->closed);
            break;
        case OBJ_TABLE: {
            obj_table_t* table = (obj_table_t*)object;
            l_mark_object((obj_t*)table);
            l_mark_table(&table->table);
            break;
        }
        case OBJ_ERROR: {
            obj_error_t* error = (obj_error_t*)object;
            l_mark_object((obj_t*)error->enclosed);
            l_mark_object((obj_t*)error->msg);
            l_mark_object((obj_t*)error);
        }
        case OBJ_ARRAY: {
            obj_array_t* array = (obj_array_t*)object;
            l_mark_object((obj_t*)array);
            l_mark_array(&array->values);
        }
        case OBJ_NATIVE:
        case OBJ_STRING:
        case OBJ_ITERATOR:
        break;
    }
}

void _free_objects() {
    obj_t* object = _mem.objects;
    while (object != NULL) {
        obj_t* next = object->next;
        _free_object(object);
        object = next;
    }

    free(_mem.gray_stack);
}

void _free_object(obj_t* object) {

    if (object == NULL) {
        return;
    }

#if defined(DEBUG_LOG_GC)

    size_t free_size = 0;

    switch (object->type) {
        case OBJ_BOUND_METHOD:
            free_size = sizeof(obj_bound_method_t);
            break;
        case OBJ_CLASS: {
            free_size = sizeof(obj_class_t);
            break;
        } 
        case OBJ_CLOSURE: {
            free_size = sizeof(obj_closure_t);
            break;
        }
        case OBJ_FUNCTION: {
            free_size = sizeof(obj_function_t);
            break;
        }
        case OBJ_INSTANCE: {
            free_size = sizeof(obj_instance_t);
            break;
        }
        case OBJ_NATIVE: {
            free_size = sizeof(obj_native_t);
            break;
        }
        case OBJ_STRING: {
            free_size = sizeof(obj_string_t);
            break;
        }
        case OBJ_UPVALUE:
            free_size = sizeof(obj_upvalue_t);
            break;
        case OBJ_TABLE:
            free_size = sizeof(obj_table_t);
            break;
        case OBJ_ERROR:
            free_size = sizeof(obj_error_t);
            break;
    }

    printf("%p free %zu type %s\n",
            (void*)object, 
            free_size,
            obj_type_to_string[object->type]
    );
#endif

    switch (object->type) {
        case OBJ_BOUND_METHOD:
            FREE(obj_bound_method_t, object);
            break;
        case OBJ_CLASS: {
            obj_class_t* klass = (obj_class_t*)object;
            l_free_table(&klass->methods);
            FREE(obj_class_t, object);
            break;
        } 
        case OBJ_CLOSURE: {
            obj_closure_t* closure = (obj_closure_t*)object;
            FREE_ARRAY(obj_upvalue_t*, closure->upvalues, closure->upvalue_count);
            FREE(obj_closure_t, object);
            break;
        }
        case OBJ_FUNCTION: {
            obj_function_t* function = (obj_function_t*)object;
            l_free_chunk(&function->chunk);
            FREE(obj_function_t, object);
            break;
        }
        case OBJ_INSTANCE: {
            obj_instance_t* instance = (obj_instance_t *)object;
            l_free_table(&instance->fields);
            FREE(obj_instance_t, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(obj_native_t, object);
            break;
        }
        case OBJ_STRING: {
            obj_string_t* string = (obj_string_t*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(obj_string_t, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(obj_upvalue_t, object);
            break;

        case OBJ_TABLE: {
            obj_table_t* table = (obj_table_t *)object;
            l_free_table(&table->table);
            FREE(obj_table_t, table);
            break;
        }
        case OBJ_ARRAY: {
            obj_array_t* array = (obj_array_t *)object;
            l_free_value_array(&array->values);
            FREE(obj_array_t, array);
            break;
        }
        case OBJ_ERROR: {
            obj_error_t* error = (obj_error_t *)object;
            _free_object((obj_t*)error->enclosed);
            _free_object((obj_t*)error->msg);
            FREE(obj_error_t, object);
            break;
        }
    }
}

// Garbage Collection

void l_register_mark_roots_cb(l_mark_roots_callback fn) { 
    if (_mem.roots_cb_count == _mem.roots_cb_capacity) {
        _mem.roots_cb_capacity *= 2;
        _mem.roots_cb = realloc(
            _mem.roots_cb,
            sizeof(l_mark_roots_callback) * _mem.roots_cb_capacity
        );
    }
    _mem.roots_cb[_mem.roots_cb_count++] = fn;
}

// remove the callback from the list
void l_unregister_mark_roots_cb(l_mark_roots_callback fn) {
    for (int i = 0; i < _mem.roots_cb_count; i++) {
        if (_mem.roots_cb[i] == fn) {
            _mem.roots_cb[i] = _mem.roots_cb[_mem.roots_cb_count - 1];
            _mem.roots_cb_count--;
            break;
        }
    }
}

void l_register_garbage_collect_cb(l_gc_callback fn) {
    if (_mem.gc_cb_count == _mem.gc_cb_capacity) {
        _mem.gc_cb_capacity *= 2;
        _mem.gc_cb = realloc(
            _mem.gc_cb,
            sizeof(l_gc_callback) * _mem.gc_cb_capacity
        );
    }
    _mem.gc_cb[_mem.gc_cb_count++] = fn;
}

// remove the garbage collection callback from the list
void l_unregister_garbage_collect_cb(l_gc_callback fn) {
    for (int i = 0; i < _mem.gc_cb_count; i++) {
        if (_mem.gc_cb[i] == fn) {
            _mem.gc_cb[i] = _mem.gc_cb[_mem.gc_cb_count - 1];
            _mem.gc_cb_count--;
            break;
        }
    }
}

void _call_mark_roots() {
    for (int i = 0; i < _mem.roots_cb_count; i++) {
        _mem.roots_cb[i]();
    }
}

void _call_garbage_collect() {
    for (int i = 0; i < _mem.gc_cb_count; i++) {
        _mem.gc_cb[i]();
    }
}

static void _trace_references() {
    while (_mem.gray_count > 0) {
        obj_t* object = _mem.gray_stack[--_mem.gray_count];
        _blacken_object(object);
    }
}

static void _sweep() {
    obj_t* previous = NULL;
    obj_t* object = _mem.objects;
    while (object != NULL) {
        if (object->is_marked) {
            object->is_marked = false;
            previous = object;
            object = object->next;
        } else {
            obj_t* unreached = object;
            object = object->next;

            if (previous != NULL) {
                previous->next = object;
            } else {
                _mem.objects = object;
            }

            _free_object(unreached);
        }
    }
}


void  l_collect_garbage() {
#if defined(DEBUG_LOG_GC)
    printf("-- gc begin\n");
    size_t before = _mem.bytes_allocated;
#endif

    _call_mark_roots();

    _trace_references();

    _call_garbage_collect();

    _sweep();

    _mem.next_gc = _mem.bytes_allocated * GC_HEAP_GROW_FACTOR;

#if defined(DEBUG_LOG_GC)
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - _mem.bytes_allocated, before, _mem.bytes_allocated,
         _mem.next_gc);
#endif

}

void l_add_object(obj_t* object) {
    object->next = _mem.objects;
    _mem.objects = object;
}

obj_t* l_get_objects() {
    return _mem.objects;
}

// bytecode deserialisation


// initialise the bytecode deserialisation allocation tracking
void l_allocate_track_init() {

    mem_track_t * track = &_mem.track;

    track->track_allocations = true;

    // allocate the initial capacity for the native lookup
    track->native_lookup.capacity = 64;
    track->native_lookup.items = ALLOCATE(native_lookup_item_t, track->native_lookup.capacity);
    track->native_lookup.count = 0;

    // allocate the initial capacity for the object lookup
    track->object_lookup.capacity = 64;
    track->object_lookup.items = ALLOCATE(object_lookup_item_t, track->object_lookup.capacity);
    track->object_lookup.count = 0;

    // allocate the initial capacity for the string lookup
    track->string_lookup.capacity = 64;
    track->string_lookup.items = ALLOCATE(string_lookup_item_t, track->string_lookup.capacity);
    track->string_lookup.count = 0;
}

// cleanup the allocation tracking data
void l_allocate_track_free() {

    mem_track_t * track = &_mem.track;

    track->track_allocations = false;

    FREE_ARRAY(native_lookup_item_t, track->native_lookup.items, track->native_lookup.capacity);

    for (int i = 0; i < track->object_lookup.count; i++) {
        object_lookup_item_t item = track->object_lookup.items[i];
        FREE_ARRAY(obj_t **, item.registered_targets, item.capacity);
    }
    track->native_lookup.count = 0;

    FREE_ARRAY(object_lookup_item_t, track->object_lookup.items, track->object_lookup.capacity);

    for (int i = 0; i < track->string_lookup.count; i++) {
        string_lookup_item_t item = track->string_lookup.items[i];
        FREE_ARRAY(obj_t **, item.registered_targets, item.capacity);
        item.count = 0;
    }

    FREE_ARRAY(string_lookup_item_t, track->string_lookup.items, track->string_lookup.capacity);
    track->string_lookup.count = 0;
}

// register a native function pointer into the tracking structure
void l_allocate_track_register_native(const char *name, void * ptr) {
    
    mem_track_t * track = &_mem.track;

    if (track->native_lookup.count + 1 > track->native_lookup.capacity) {
        size_t old_capacity = track->native_lookup.capacity;
        track->native_lookup.capacity = GROW_CAPACITY(old_capacity);
        track->native_lookup.items = GROW_ARRAY(
            native_lookup_item_t,
            track->native_lookup.items,
            old_capacity,
            track->native_lookup.capacity
        );
    }

    // copy the input string and assign it the lookup item below
    // this will be automatically cleaned up with the VM
    obj_string_t * str = l_new_string(name);
    
    track->native_lookup.items[track->native_lookup.count] = (native_lookup_item_t) {
        .name = str->chars,
        .native = ptr
    };

    track->native_lookup.count++;
}

// get a native function pointer
void * l_allocate_track_get_native_ptr(const char *name) {

    mem_track_t * track = &_mem.track;

    for (int i = 0; i < track->native_lookup.count; i++) {
        native_lookup_item_t * item = &track->native_lookup.items[i];
        if (item->name != NULL && strcmp(item->name, name) == 0) {
            return item->native;
        }
    }

    return NULL;
}

// get a native function name
const char * l_allocate_track_get_native_name(void * ptr) {
    mem_track_t * track = &_mem.track;

    for (int i = 0; i < track->native_lookup.count; i++) {
        native_lookup_item_t * item = &track->native_lookup.items[i];
        if (item->native != NULL && item->native == ptr) {
            return item->name;
        }
    }

    return NULL;
}

// insert a new allocation for tracking into the allocation lookup table
object_lookup_item_t * _insert_or_get_object_lookup_item(uintptr_t id) {

    mem_track_t * track = &_mem.track;

    // Check if the item already exists, and return if found
    for (int i = 0; i < track->object_lookup.count; i++) {
        if (track->object_lookup.items[i].id == id) {
            return &track->object_lookup.items[i];
        }
    }

    // otherwise, insert a new item
    if (track->object_lookup.count + 1 > track->object_lookup.capacity) {
        size_t old_capacity = track->object_lookup.capacity;
        track->object_lookup.capacity = GROW_CAPACITY(old_capacity);
        track->object_lookup.items = GROW_ARRAY(
            object_lookup_item_t,
            track->object_lookup.items,
            old_capacity,
            track->object_lookup.capacity
        );
    }

    track->object_lookup.items[track->object_lookup.count++] = (object_lookup_item_t) {
        .id = id,
        .address = NULL,
        .registered_targets = NULL,
    };

    // return the newly inserted item
    return &track->object_lookup.items[track->object_lookup.count - 1];
}

// insert a new allocation for tracking into the allocation lookup table
void l_allocate_track_register(uintptr_t id, obj_t *address) {
    
    object_lookup_item_t * item = _insert_or_get_object_lookup_item(id);

    // assign the new address
    item->address = address;
#if defined(LINK_DEBUGGING)
    // display the registered id
    printf("Registered allocation id: %lu -> %p\n", id, item->address);
#endif

}

// register interest in a target address
void l_allocate_track_target_register(uintptr_t id, obj_t **target) {
    
    // if the id doesn't in the lookup table, then we need to add it
    // this is because the target address may be set before the object is
    // allocated, so we need to register the target address before the object

    object_lookup_item_t * item = _insert_or_get_object_lookup_item(id);

    // check if the target is already registered, ignore the second registration
    for (size_t i = 0; i < item->count; i++) {
        if (item->registered_targets[i] == (obj_t **)target) {
            return;
        }
    }

    // otherwise, insert a new target
    if (item->count + 1 > item->capacity) {
        size_t old_capacity = item->capacity;
        item->capacity = GROW_CAPACITY(old_capacity);
        item->registered_targets = GROW_ARRAY(
            obj_t **,
            item->registered_targets,
            old_capacity,
            item->capacity
        );
        for (size_t i = item->count; i < item->capacity; i++) {
            item->registered_targets[i] = NULL;
        }
    }

    // register a new interest in the target address
    item->registered_targets[item->count] = (obj_t **)target;
    // ensure that the target is NULL'd
    *item->registered_targets[item->count] = NULL;
#if defined(LINK_DEBUGGING)
    printf("Registering interest in id: %lu -> %p\n", id, *target);
#endif
    item->count++;
}


string_lookup_item_t * _insert_or_get_string_lookup_item(uint32_t hash) {

    string_lookup_t * lookup = &_mem.track.string_lookup;

    // Check if the item already exists, and return if found
    for (size_t i = 0; i < lookup->count; i++) {
        if (lookup->items[i].hash == hash) {
            return &lookup->items[i];
        }
    }

    // otherwise, insert a new item
    if (lookup->count + 1 > lookup->capacity) {
        size_t old_capacity = lookup->capacity;
        lookup->capacity = GROW_CAPACITY(old_capacity);
        lookup->items = GROW_ARRAY(
            string_lookup_item_t,
            lookup->items,
            old_capacity,
            lookup->capacity
        );
    }

    lookup->items[lookup->count++] = (string_lookup_item_t) {
        .hash = hash,
        .registered_targets = NULL,
    };

    // return the newly inserted item
    return &lookup->items[lookup->count - 1];
}

// insert a new string for tracking into the allocation lookup table
void l_allocate_track_string_register(uint32_t hash, obj_string_t *address) {
    
    string_lookup_item_t * item = _insert_or_get_string_lookup_item(hash);

    // assign the new address
    item->address = address;

#if defined(LINK_DEBUGGING)
    // display the registered id
    printf("Registered string hash: %lu -> %s [%d]\n", hash, item->address->chars, item->count);
#endif
}

// register interest in a string by hash
void l_allocate_track_string_target_register(uint32_t hash, obj_string_t **target) {

    string_lookup_item_t * item = _insert_or_get_string_lookup_item(hash);

    // check if the target is already registered, ignore the second registration
    for (size_t i = 0; i < item->count; i++) {
        if ((obj_t**)(item->registered_targets[i]) == (obj_t **)target) {
            return;
        }
    }

    // otherwise, insert a new target
    if (item->count + 1 > item->capacity) {
        size_t old_capacity = item->capacity;
        item->capacity = GROW_CAPACITY(old_capacity);
        item->registered_targets = GROW_ARRAY(
            obj_string_t **,
            item->registered_targets,
            old_capacity,
            item->capacity
        );
        for (size_t i = item->count; i < item->capacity; i++) {
            item->registered_targets[i] = NULL;
        }
    }

    // register a new interest in the string
    item->registered_targets[item->count] = (obj_string_t **)target;
    // ensure that the target is NULL'd
    *item->registered_targets[item->count] = NULL;
#if defined(LINK_DEBUGGING)
    printf("Registering interest in string hash: %lu -> %p\n", hash, target);
#endif
    item->count++;
}

// link the registered targets to the allocated addresses for those targets
void l_allocate_track_link_targets() {
    mem_track_t * track = &_mem.track;

#if defined(LINK_DEBUGGING)
    printf("linking:\n");
#endif

    // link objects
#if defined(LINK_DEBUGGING)
        printf("linking: pointers\n");
#endif

    for (size_t i = 0; i < track->object_lookup.count; i++) {
        object_lookup_item_t * item = &track->object_lookup.items[i];
        
        if (item->address == NULL) {
            continue;
        }
        for (size_t j = 0; j < item->count; j++) {
            // print the target address and the linked address
            obj_t **target = item->registered_targets[j];
#if defined(LINK_DEBUGGING)
            printf("\tlinking alloc: %lu: p: %p -> target:%p\n", item->id, item->address, *target);
#endif
            *target = item->address;
        }
    }

    // link strings
#if defined(LINK_DEBUGGING)
        printf("linking: strings\n");
#endif
    for (size_t i = 0; i < track->string_lookup.count; i++) {
        string_lookup_item_t * item = &track->string_lookup.items[i];

#if defined(LINK_DEBUGGING)
            printf("\tfor string: %lu: %s\n", item->hash, (item->address == NULL) ? "NULL: skipping" : item->address->chars );
#endif   

        if (item->address == NULL)
            continue;

        for (int j = 0; j < item->count; j++) {
            // set the string pointer into the registered targets
            obj_string_t **target = item->registered_targets[j];
#if defined(LINK_DEBUGGING)
            printf("\t\tlinking string: %lu: p: %p -> target:%p\n", item->hash, item->address, *target);
#endif
            *target = item->address;
        }
    }
}

