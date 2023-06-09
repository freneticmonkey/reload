#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>


#include "memory/allocator.h"
#include "filetracker/filetracker.h"
#include "module/interface.h"

// uint64_t _get_time_ms() {
//     struct timespec current_time;
//     clock_gettime(CLOCK_REALTIME, &current_time);
//     return current_time.tv_sec * 1000 + current_time.tv_nsec / 1000000;
// }

typedef struct r_filetracker {
    r_module_interface *modules[MAX_MODULES];
    uint32_t            count;
} r_filetracker;

// Create a new filetracker instance
r_filetracker * r_filetracker_create() {

    // Allocate memory for the filetracker
    r_filetracker *filetracker = MALLOC(r_filetracker, 1);
    filetracker->count = 0;

    return filetracker;
}

// clean up the filetracker instance
void r_filetracker_destroy(r_filetracker *filetracker) {
    
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
}

// remove a module from being tracked
void r_filetracker_remove_module(r_filetracker *filetracker, const char *lib_path) {

    // Find the module in the filetracker
    for (uint32_t i = 0; i < filetracker->count; i++) {
        r_module_properties *props = &filetracker->modules[i]->properties;
        if (strcmp(props->library_path, lib_path) == 0) {

            // shuffle the modules down and decrement the count
            for (uint32_t j = i; j < filetracker->count - 1; j++) {
                filetracker->modules[j] = filetracker->modules[j + 1];
            }
            filetracker->count--;
        }
    }
}

// check if any modules have been modified
void r_filetracker_check(r_filetracker *filetracker) {

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
}