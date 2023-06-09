#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include "lib/basic/basic.h"

#include "lib/filetracker/filetracker.h"
#include "lib/module/module.h"
#include "lib/time/time.h"

static bool finished = false;

void sigterm_handler(int signum) {
    printf("Received SIGTERM signal\n");
    finished = true;
}

void sigint_handler(int signum) {
    printf("Received SIGINT signal\n");
    finished = true;
}

int main(int argc, const char* argv[]) {

    printf("Starting Reload ...\n");

    r_time_init(); 

    // register signals
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigint_handler);

    // Create a module lifecycle instance
    r_module_lifecycle *lifecycle = r_module_lifecycle_create();

    // Create a filetracker instance
    r_filetracker *filetracker = r_filetracker_create();

    // Now create an instance of the basic module
    // This is a local but it goes out of scope with everything else
    r_module_interface *basic_interface = &(r_module_interface){
        .properties = (r_module_properties){
            .name = "basic",
            .library_path = "./build/libbasic.dylib",
            .library_handle = NULL,
            .last_modified = 0,
            .needs_reload = false,
        },
    };

    // Add the basic module to the lifecycle
    r_module_lifecycle_register(lifecycle, basic_interface);

    // Add the basic module to the filetracker
    r_filetracker_add_module(filetracker, basic_interface);
    
    float target = 60.f;
    float frame_target = 1000.f / target;
    float last = _get_time_ms();

    // loop until we're finished
    while (!finished) {

        float now = _get_time_ms();
        float delta_time = now - last;
        last = now;
        float remaining = frame_target - delta_time;
        
        // Check for changes
        r_filetracker_check(filetracker);

        // Update
        r_module_lifecycle_update(lifecycle, delta_time);
        
        // sleep the remaining amount of frame time
        if (remaining > 0.f) {
            sleep(remaining/1000.f);
        }
    }

    // Destroy the filetracker instance
    r_filetracker_destroy(filetracker);

    // Destroy the module lifecycle instance
    r_module_lifecycle_destroy(lifecycle);
    
    // Load the library
    printf("Reload finished.\n");

    return 0;
}
