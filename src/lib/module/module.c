#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

#include "memory/allocator.h"
#include "module/module.h"

typedef struct {
    r_module_interface *interfaces[MAX_MODULES];
    uint32_t            count;
} r_module_interface_array;

typedef struct r_module_lifecycle {
    r_module_interface_array modules;
    // void    *persistent_memory;
    // uint32_t persistent_memory_size;
} r_module_lifecycle;

void _module_destroy(r_module_interface *interface);
void _module_reload(r_module_interface *interface);

// Create a new lifecyle instance
r_module_lifecycle * r_module_lifecycle_create() {

    // Allocate memory for the lifecycle
    r_module_lifecycle *lifecycle = MALLOC(r_module_lifecycle, 1);
    lifecycle->modules.interfaces[0] = NULL;
    
    return lifecycle;

}
void r_module_lifecycle_destroy(r_module_lifecycle *lifecycle) {

    // clean up any instances
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = lifecycle->modules.interfaces[i];
        
        _module_destroy(interface);
    }

    // Free the lifecycle
    FREE(r_module_lifecycle, lifecycle);
}

void r_module_lifecycle_register(r_module_lifecycle *lifecycle, r_module_interface *interface) {
    
        // Check if the interface is already registered
        for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
            if (lifecycle->modules.interfaces[i] == interface) {
                return;
            }
        }

        // Check if we've hit MAX_MODULES
        if (lifecycle->modules.count == MAX_MODULES) {
            return;
        }

        // insert the module at the end of the array
        lifecycle->modules.interfaces[lifecycle->modules.count++] = interface;
    
        // Load the module
        _module_reload(interface);
}
void r_module_lifecycle_unregister(r_module_lifecycle *lifecycle, r_module_interface *interface) {

    // Remove the interface from the lifecycle
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        if (lifecycle->modules.interfaces[i] == interface) {

            // Clean up the interface and unload the library
            _module_destroy(interface);

            // shuffle the modules down and decrement the count
            for (uint32_t j = i; j < lifecycle->modules.count - 1; j++) {
                lifecycle->modules.interfaces[j] = lifecycle->modules.interfaces[j + 1];
            }
            lifecycle->modules.count--;
            break;
        }
    }
}

void r_module_lifecycle_update(r_module_lifecycle *lifecycle, float delta_time) {

    // Update all the interfaces
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = lifecycle->modules.interfaces[i];

        // Check if the interface needs to be reloaded
        if (interface->properties.needs_reload) {
            _module_reload(interface);
        }

        // Now update the module
        if (interface->cb.update) {
            interface->cb.update(&interface->properties, delta_time);
        }
    }
}

void _module_destroy(r_module_interface *interface) {
    if (interface->cb.destroy) {
        interface->cb.destroy(&interface->properties);
    }

    // unload the library
    dlclose(interface->properties.library_handle);
    interface->properties.library_handle = NULL;
}

void _module_reload(r_module_interface *interface) {

    bool call_reload = false;

    // if the library has been loaded before
    if (interface->properties.library_handle != NULL) {
        call_reload = true;

        // fire the unload first to allow the module to get itself ready
        if (interface->cb.on_unload) {
            interface->cb.on_unload(&interface->properties);
        }

        // unload the library
        dlclose(interface->properties.library_handle);
        interface->properties.library_handle = NULL;
    }

    // load the library
    interface->properties.library_handle = dlopen(interface->properties.library_path, RTLD_NOW | RTLD_LOCAL);

    if (!interface->properties.library_handle) {
        // display an error and return
        fprintf(stderr, "%s\n", dlerror());
        return;
    }   

    // Obtain the module's entry points
    interface->cb = (r_module_callbacks){
        .init       = dlsym(interface->properties.library_handle, "init"),
        .destroy    = dlsym(interface->properties.library_handle, "destroy"),
        .update     = dlsym(interface->properties.library_handle, "update"),
        .on_unload  = dlsym(interface->properties.library_handle, "on_unload"),
        .on_reload  = dlsym(interface->properties.library_handle, "on_reload")
    };

    // if this is the first time that we're calling this module
    if (!call_reload) {
        if (interface->cb.init) {
            interface->cb.init(&interface->properties);
        }
    } else {
        if (interface->cb.on_reload) {
            interface->cb.on_reload(&interface->properties);
        }
    }

    interface->properties.needs_reload = false;
}