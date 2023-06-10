#ifndef _MODULE_INTERFACE_H_
#define _MODULE_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_MODULES 64

// Module Interface is a structure of function pointers
// which provide access to lifecycle functions for a library
// which is being reloaded.
//
// The main process will allow the library to register persistent memory
// which can be handed back to the library after reload
// to allow the library to maintain state across reloads.
//
//
// The interface will allow the library to define the following functions:
// - init
// - destroy
// - update
// - on unload
// - on reload

typedef struct r_module_properties {
    // module properties
    char * name;
    void * persistent_memory;

    char *   library_path;
    char *   library_files_root;
    void *   library_handle;
    uint64_t last_modified;
    bool     needs_reload;
    bool     files_changed;
} r_module_properties;

typedef struct r_module_callbacks {
    bool (*init)(r_module_properties *lib_interface);
    bool (*destroy)(r_module_properties *lib_interface);
    bool (*update)(r_module_properties *lib_interface, float delta_time);
    bool (*on_unload)(r_module_properties *lib_interface);
    bool (*on_reload)(r_module_properties *lib_interface);
} r_module_callbacks;

typedef struct r_module_interface {
    // lifecycle properties
    r_module_properties properties;

    // entry points
    r_module_callbacks cb;
} r_module_interface;

r_module_interface * r_module_interface_create();
void r_module_interface_destroy(r_module_interface *lib_interface);

#endif