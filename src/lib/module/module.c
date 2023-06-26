#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "memory/allocator.h"
#include "module/module.h"

typedef struct {
    r_module_interface interfaces[MAX_MODULES];
    uint32_t           count;
} r_module_interface_array;

typedef struct r_module_lifecycle {
    r_module_interface_array modules;
    // void    *persistent_memory;
    // uint32_t persistent_memory_size;
} r_module_lifecycle;

void _module_destroy(r_module_interface *interface);
void _module_reload(r_module_interface *interface);
void _module_rebuild(r_module_interface *interface);

// Create a new lifecyle instance
r_module_lifecycle * r_module_lifecycle_create() {

    // Allocate memory for the lifecycle
    r_module_lifecycle *lifecycle = MALLOC(r_module_lifecycle, 1);
    lifecycle->modules.count = 0;
    return lifecycle;

}
void r_module_lifecycle_destroy(r_module_lifecycle *lifecycle) {

    // clean up any instances
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = &lifecycle->modules.interfaces[i];
        
        _module_destroy(interface);
    }

    // Free the lifecycle
    FREE(r_module_lifecycle, lifecycle);
}

r_module_interface * r_module_lifecycle_register(r_module_lifecycle *lifecycle, r_module_properties properties) {

        // Check if we've hit MAX_MODULES
        if (lifecycle->modules.count == MAX_MODULES) {
            return NULL;
        }

        // Check if the interface is already registered
        for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
            // if the module name already exists
            if (strcmp(lifecycle->modules.interfaces[i].properties.name, properties.name) == 0) {
                return NULL;
            }
        }

        r_module_interface *interface = &lifecycle->modules.interfaces[lifecycle->modules.count++];
        interface->properties = properties;
        
        // Load the module
        _module_reload(interface);

        return interface;
}
void r_module_lifecycle_unregister(r_module_lifecycle *lifecycle, r_module_interface *interface) {

    // Remove the interface from the lifecycle
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        if (&lifecycle->modules.interfaces[i] == interface) {

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

void r_module_lifecycle_pre_frame(r_module_lifecycle *lifecycle, float delta_time) {

    // Update all the interfaces
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = &lifecycle->modules.interfaces[i];

        // Now update the module
        if (interface->cb.pre_frame) {
            interface->cb.pre_frame(&interface->properties, delta_time);
        }
    }
}

void r_module_lifecycle_update(r_module_lifecycle *lifecycle, float delta_time) {

    // Update all the interfaces
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = &lifecycle->modules.interfaces[i];

        // Now update the module
        if (interface->cb.update) {
            interface->cb.update(&interface->properties, delta_time);
        }
    }
}

void r_module_lifecycle_ui_update(r_module_lifecycle *lifecycle, float delta_time) {

    // Update all the interfaces
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = &lifecycle->modules.interfaces[i];

        // Run the UI update for the module
        if (interface->cb.ui_update) {
            interface->cb.ui_update(&interface->properties, delta_time);
        }
    }
}

void r_module_lifecycle_post_frame(r_module_lifecycle *lifecycle, float delta_time) {

    // Update all the interfaces
    for (uint32_t i = 0; i < lifecycle->modules.count; i++) {
        r_module_interface *interface = &lifecycle->modules.interfaces[i];

        // Check if the files have been modified
        if (interface->properties.files_changed) {
            _module_rebuild(interface);
        }

        // Check if the interface needs to be reloaded
        if (interface->properties.needs_reload) {
            _module_reload(interface);
        }

        // Now update the module
        if (interface->cb.post_frame) {
            interface->cb.post_frame(&interface->properties, delta_time);
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

    FREE(char, interface->properties.name);
    FREE(char, interface->properties.library_path);
    FREE(char, interface->properties.library_files_root);
    
}

void _module_reload(r_module_interface *interface) {

    bool call_reload = false;

    // if the library has been loaded before
    if (interface->properties.library_handle != NULL) {
        call_reload = true;

        // fire the unload first to allow the module to get itself ready
        if (interface->cb.on_unload) {
            interface->cb.on_unload(&interface->properties);

            // check if the data version has changed
            if (interface->properties.memory.data_version != interface->properties.previous_data_version) {
                interface->properties.previous_data_version = interface->properties.memory.data_version;

                // disabling reload will force the module to be re-initalised
                call_reload = false;

                // call the destructor for the previous data version
                if (interface->properties.memory.destroy) {
                    interface->properties.memory.destroy(&interface->properties);
                }
            }
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

        .pre_frame  = dlsym(interface->properties.library_handle, "pre_frame"),
        .update     = dlsym(interface->properties.library_handle, "update"),
        .ui_update  = dlsym(interface->properties.library_handle, "ui_update"),
        .post_frame = dlsym(interface->properties.library_handle, "post_frame"),
        
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

void _module_rebuild(r_module_interface *interface) {
    // Now run a build of the module
    char *command = NULL;
    asprintf(&command, "./build/build module:%s", interface->properties.name);

    // create a new subprocess to run the build
    pid_t buildPid = fork();
    
    if (buildPid == -1) {
        fprintf(stderr, "Failed to fork process\n");
        return;
    }

    // if we're the child process
    if (buildPid == 0) {

        // This won't be freed but the whole process will be freed on exit
        FILE * buildOutput = popen(command, "r");

        if (buildOutput == NULL) {
            fprintf(stderr, "Failed to run command\n");

            exit(1);
        }

        // read the output
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), buildOutput) != NULL) {
            printf("%s", buffer);
        }

        // close the output
        pclose(buildOutput);

        exit(0);
    } else {
        printf("Triggered background build of module: %s\n", interface->properties.name);

    }

    // cleanup the command now that we don't need it anymore    
    free(command);
    
    // now the module should be detected as having been reloaded
    interface->properties.files_changed = false;
}
