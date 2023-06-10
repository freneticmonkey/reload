// mylib.c
#include <stdio.h>

#include "raylib/raylib.h"

#include "memory/allocator.h"


#include "basic.h"

typedef struct _basic_internals {
    Vector2  ballPosition;
    Camera3D camera;
} _basic_internals;

static _basic_internals *_internals = NULL;

// initialise the _basic_internals data
void * _basic_int_create(r_module_properties *props) {

    _basic_internals *bint = MMALLOC(_basic_internals, 1);

    if (bint == NULL)
        return NULL;

    bint->ballPosition = (Vector2){ 
        .x = (float)800/2, 
        .y = (float)450/2 
    };

    bint->camera = (Camera3D){
        .position = (Vector3){ 10.0f, 10.0f, 10.0f }, // Camera position
        .target = (Vector3){ 0.0f, 0.0f, 0.0f },      // Camera looking at point
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },          // Camera up vector (rotation towards target)
        .fovy = 45.0f,                                // Camera field-of-view Y
        .projection = CAMERA_PERSPECTIVE,             // Camera projection type
    };
    return bint;
}

// destroy the _basic_internals data
void _basic_int_destroy(r_module_properties *props) {
    _basic_internals *bint = (_basic_internals *)props->memory.p_mem;
    MFREE(_basic_internals, bint);
}

bool init(r_module_properties *props) {
    printf("basic: init()\n");
    
    props->memory.p_mem = _internals = _basic_int_create(props);
    props->memory.create = _basic_int_create;
    props->memory.destroy = _basic_int_destroy;
    // alloc and free - already set
        
    return true;
}

bool destroy(r_module_properties *props) {
    printf("basic: destroy()\n");
    _basic_int_destroy(props);
    return true;
}

bool update(r_module_properties *props, float delta_time) {
    static float el = 0.f;
    el += delta_time;
    
    // animate the text across the screen
    static int xPos = 0;

    int speed = 300;
        
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

    BeginMode3D(_internals->camera);
    DrawGrid(10, 1.0f);
    EndMode3D();

    return true;
}

bool on_unload(r_module_properties *props) {
    printf("basic: on_unload()\n");
    return true;
}

bool on_reload(r_module_properties *props) {
    printf("basic: on_reload()\n");
    _internals = props->memory.p_mem;
    return true;
}
