#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>


#include "memory/allocator.h"
#include "module/interface.h"

#define USING_NOTIFY 1

#include "filetracker/filetracker.h"

#ifdef USING_NOTIFY
#include "filetracker/notify/notify.h"
#endif

typedef struct r_filetracker {
    r_module_interface *modules[MAX_MODULES];
    uint32_t            count;
} r_filetracker;

// Create a new filetracker instance
r_filetracker * r_filetracker_create() {

    // Allocate memory for the filetracker
    r_filetracker *filetracker = MALLOC(r_filetracker, 1);
    filetracker->count = 0;

#ifdef USING_NOTIFY
    // Initialize the notify system
    r_file_notify_init();
#endif

    return filetracker;
}

// clean up the filetracker instance
void r_filetracker_destroy(r_filetracker *filetracker) {
    
#ifdef USING_NOTIFY
    // Destroy the notify system
    r_file_notify_destroy();
#endif

    // reset the count
    filetracker->count = 0;   

    // Free the filetracker
    FREE(r_filetracker, filetracker);
}

// add a module to be tracked
void r_filetracker_add_module(r_filetracker *filetracker, r_module_interface *module) {

    // Add the module to the filetracker
    if (filetracker->count == MAX_MODULES) {
        return;
    }
    filetracker->modules[filetracker->count++] = module;

#ifdef USING_NOTIFY
    // Add the module to the notify system
    r_file_notifier_create(module->properties.library_files_root, &module->properties.files_changed);
#endif

}

// remove a module from being tracked
void r_filetracker_remove_module(r_filetracker *filetracker, const char *lib_path) {

    // Find the module in the filetracker
    for (uint32_t i = 0; i < filetracker->count; i++) {
        r_module_properties *props = &filetracker->modules[i]->properties;
        if (strcmp(props->library_path, lib_path) == 0) {

#ifdef USING_NOTIFY
            // Remove the module from the notify system
            r_file_notifier_destroy(&props->files_changed);
#endif

            // shuffle the modules down and decrement the count
            for (uint32_t j = i; j < filetracker->count - 1; j++) {
                filetracker->modules[j] = filetracker->modules[j + 1];
            }
            filetracker->count--;
        }
    }
}

// check if any modules have been modified
void r_filetracker_check(r_filetracker *filetracker, float delta_time) {

    static float time_since_last_check = 0.0f;
    time_since_last_check += delta_time;

    // Check if we should check for modified modules
    if (time_since_last_check < CHECK_FREQUENCY_S * 1000.f) {
        return;
    }
    time_since_last_check = 0.0f;

    // Check if any modules have been modified and need reloading
    for (uint32_t i = 0; i < filetracker->count; i++) {
        r_module_properties *props = &filetracker->modules[i]->properties;
        struct stat statbuf;

        if (stat(props->library_path, &statbuf) == 0) {

            if (props->last_modified != statbuf.st_mtime) {

                props->last_modified = statbuf.st_mtime;
                props->needs_reload = true;
            }
        } else {
            perror("failed to stat library");
        }
    }

#ifdef USING_NOTIFY
    // Pump the notify system - 100ms timeout for now
    r_file_notify_update(0.1);
#endif

}