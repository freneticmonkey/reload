// mylib.c
#include <stdio.h>

#include "raylib/raylib.h"

#include "memory/allocator.h"


#include "basic.h"


typedef enum {
    ENTITY_TYPE_CUBE,
    ENTITY_TYPE_SPHERE,
    ENTITY_TYPE_PLANE,
    ENTITY_TYPE_COUNT
} entity_type;

typedef struct entity_t {
    Vector3 position;
    Vector3 dim;
    Color   colour;
    int     type;
} entity_t;

void _draw_entity(entity_t *entity) {
    switch(entity->type) {
        case ENTITY_TYPE_CUBE:
            DrawCube(entity->position, entity->dim.x, entity->dim.y, entity->dim.z, entity->colour);
            DrawCubeWires(entity->position, entity->dim.x, entity->dim.y, entity->dim.z, BLACK);
            break;
        case ENTITY_TYPE_SPHERE:
            DrawSphere(entity->position, entity->dim.x, entity->colour);
            DrawSphereWires(entity->position, entity->dim.x, 16, 16, BLACK);
            break;
        case ENTITY_TYPE_PLANE:
            DrawPlane(entity->position, (Vector2){entity->dim.x, entity->dim.y}, entity->colour);
            break;
        default:
            break;
    }
}

#define MAX_ENTITIES 32

typedef struct _basic_internals {
    Camera3D camera;
    entity_t _entities[MAX_ENTITIES];
    int _entity_count;
} _basic_internals;

static _basic_internals *_mem = NULL;

static bool _cursor_locked = false;

// initialise the _basic_internals data
void * _basic_int_create(r_module_properties *props) {

    _basic_internals *bint = MMALLOC(_basic_internals, 1);

    if (bint == NULL)
        return NULL;

    bint->_entity_count = 0;

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

void _add_entity(entity_t ent) {
    _mem->_entities[_mem->_entity_count++] = ent;
}

bool init(r_module_properties *props) {
    printf("basic: init()\n");
    
    _mem = _basic_int_create(props);
    props->memory.p_mem = (void *)_mem;
    props->memory.create = _basic_int_create;
    props->memory.destroy = _basic_int_destroy;
    // alloc and free - already set

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            _add_entity((entity_t){
                .type = (i + j) % ENTITY_TYPE_COUNT - 1,
                .position = (Vector3){ i * 1.5f, 0.0f, j * 1.5f },
                .dim = (Vector3){ 1.0f, 1.0f, 1.0f },
                .colour = MAGENTA,
            });
        }
    }

    // 
        
    return true;
}

bool destroy(r_module_properties *props) {
    printf("basic: destroy()\n");
    _basic_int_destroy(props);
    return true;
}

bool pre_frame(r_module_properties *props, float delta_time) {
    return true;
}

bool update(r_module_properties *props, float delta_time) {
    static float el = 0.f;
    el += delta_time;
    
    if (_cursor_locked) {
        UpdateCamera(&_mem->camera, CAMERA_FREE);
    }    

    if (IsKeyDown('Z')) {
        _mem->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
        _mem->camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
        _mem->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    }

    BeginMode3D(_mem->camera);
    DrawGrid(10, 1.0f);

    // Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    for (int i = 0; i < _mem->_entity_count; i++) {
        _draw_entity(&_mem->_entities[i]);
    }

    // DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);
    // DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);

    EndMode3D();

    return true;
}

bool ui_update(r_module_properties *props, float delta_time) {
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

    // if (IsKeyDown(KEY_RIGHT)) _mem->ballPosition.x += ballSpeed;
    // if (IsKeyDown(KEY_LEFT)) _mem->ballPosition.x -= ballSpeed;
    // if (IsKeyDown(KEY_UP)) _mem->ballPosition.y -= ballSpeed;
    // if (IsKeyDown(KEY_DOWN)) _mem->ballPosition.y += ballSpeed;


    // DrawCircleV(_mem->ballPosition, 30, MAROON);

    // Control Cursor
    bool previous = _cursor_locked;
    if (IsKeyDown(KEY_CAPS_LOCK) )
        _cursor_locked = !_cursor_locked;

    if (previous != _cursor_locked) {
        if (_cursor_locked)
            DisableCursor();
        else
            EnableCursor();
    } else {
        if (_cursor_locked)
            DrawText("Press CAPS LOCK to enable cursor", 10, 10, 20, DARKGRAY);\
        else
            DrawText("Press CAPS LOCK to disable cursor", 10, 10, 20, GRAY);
    }


    DrawFPS(10, 10);
}

bool post_frame(r_module_properties *props, float delta_time) {
    return true;
}

bool on_unload(r_module_properties *props) {
    printf("basic: on_unload()\n");
    props->memory.data_version = 2;

    return true;
}

bool on_reload(r_module_properties *props) {
    printf("basic: on_reload()\n");
    _mem = props->memory.p_mem;
    return true;
}
