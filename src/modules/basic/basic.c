// mylib.c
#include <stdio.h>

#include "basic.h"

#include "memory/allocator.h"

#include "../../ext/raylib/raylib.h"

typedef struct _basic_internals {
    Vector2 ballPosition;
} _basic_internals;

static _basic_internals *_internals = NULL;

bool init(r_module_properties *lib_interface) {
    printf("basic: init()\n");

    _internals = MALLOC(_basic_internals, 1);

    _internals->ballPosition = (Vector2){ 
        x: (float)800/2, 
        y: (float)450/2 
    };

    lib_interface->persistent_memory = _internals;

    return true;
}

bool destroy(r_module_properties *lib_interface) {
    printf("basic: destroy()\n");
    FREE(_basic_internals, _internals);
    return true;
}

bool update(r_module_properties *lib_interface, float delta_time) {
    static float el = 0.f;
    el += delta_time;
    
    // animate the text across the screen
    static int xPos = 0;

    int speed = 200;
    
    xPos += (int)(speed * (delta_time/1000.0f));

    if (xPos > 800) {
        xPos = 10;
    }
    char text[100];
    sprintf(text, "From Basic! [%d] [%f]", xPos, delta_time);

    DrawText("Smoother", xPos, 300, 20, LIGHTGRAY);
    
    float ballSpeed = 2.f;

    if (IsKeyDown(KEY_RIGHT)) _internals->ballPosition.x += ballSpeed;
    if (IsKeyDown(KEY_LEFT)) _internals->ballPosition.x -= ballSpeed;
    if (IsKeyDown(KEY_UP)) _internals->ballPosition.y -= ballSpeed;
    if (IsKeyDown(KEY_DOWN)) _internals->ballPosition.y += ballSpeed;

    DrawCircleV(_internals->ballPosition, 30, MAROON);

    return true;
}

bool on_unload(r_module_properties *lib_interface) {
    printf("basic: on_unload()\n");
    return true;
}

bool on_reload(r_module_properties *lib_interface) {
    printf("basic: on_reload()\n");
    _internals = lib_interface->persistent_memory;
    return true;
}
