#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modules/basic/basic.h"

#include "lib/module/helper.h"
#include "lib/time/time.h"

#include "ext/raylib/raylib.h"

#define MAX_FPS 60.f

static bool finished = false;

void signal_handler(int signum) {
    switch(signum) {
        case SIGINT:
            printf("Received SIGINT signal\n");
            break;
        case SIGTERM:
            printf("Received SIGTERM signal\n");
            break;
        default:
            break;
    }
    finished = true;
}

int main(int argc, const char* argv[]) {

    printf("Starting Reload ...\n");

    r_time_init(MAX_FPS);

    // register signals`
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Create module lifecycle and filetracker
    r_module_create();

    // register basic module
    r_module_add("basic");


    InitWindow(800, 450, "Reload");
    SetTargetFPS(MAX_FPS);   
    // loop until we're finished
    while (!finished || !WindowShouldClose()) {

        float delta_time = r_time_get_delta();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Reload", 190, 200, 20, LIGHTGRAY);

        r_module_update(delta_time);

        EndDrawing();

        // r_time_sleep_remaining();
    }

    CloseWindow();

    // Clean up modules
    r_module_destroy();
    
    // Load the library
    printf("Reload finished.\n");

    return 0;
}
