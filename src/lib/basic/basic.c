// mylib.c
#include <stdio.h>

#include "basic.h"


bool init(r_module_properties *lib_interface) {
    printf("basic: init()\n");
    return true;
}

bool destroy(r_module_properties *lib_interface) {
    printf("basic: destroy()\n");
    return true;
}

bool update(r_module_properties *lib_interface, float delta_time) {
    static float el = 0.f;
    el += delta_time;
    if (el > 5000.f) {
        printf("basic: update([%4.2f])\n", el);
        el = 0.f;
    }
    return true;
}

bool on_unload(r_module_properties *lib_interface) {
    printf("basic: on_unload()\n");
    return true;
}

bool on_reload(r_module_properties *lib_interface) {
    printf("basic: on_reload()\n");
    return true;
}
