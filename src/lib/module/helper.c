
#include <stdlib.h>
#include <stdio.h>

#include "filetracker/filetracker.h"

#include "module/helper.h"
#include "module/module.h"

static r_module_lifecycle *lifecycle;
static r_filetracker *filetracker;

void r_module_create() {
    // Create a module lifecycle instance
    lifecycle = r_module_lifecycle_create();

    // Create a filetracker instance
    filetracker = r_filetracker_create();
}

void r_module_add(const char *module_name) {
    
    // Now create an instance of the basic module and add it to the module lifecycles
    r_module_interface *basic_interface = r_module_lifecycle_register(
        lifecycle,
        (r_module_properties){
            .library_handle = NULL,
            .last_modified = 0,
            .needs_reload = false,
        }
    );

    // allocate memory for the module name, library path, and library files root
    asprintf(&basic_interface->properties.name, "%s", module_name);

    // TODO: Make this platform independent, i.e. so for linux and dll for windows
    asprintf(&basic_interface->properties.library_path, "./build/lib%s.dylib", module_name);
    
    asprintf(&basic_interface->properties.library_files_root, "./src/modules/%s", module_name);

    // Add the basic module to the filetracker
    r_filetracker_add_module(filetracker, basic_interface);
}

void r_module_update(float delta_time) {
    // Check for module changes
    r_filetracker_check(filetracker);

    // Update
    r_module_lifecycle_update(lifecycle, delta_time);
}

void r_module_destroy() {
    // Destroy the filetracker instance
    r_filetracker_destroy(filetracker);

    // Destroy the module lifecycle instance
    r_module_lifecycle_destroy(lifecycle);
}
