// basic.h
#ifndef BASIC_H
#define BASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "module/interface.h"

bool init(r_module_properties *lib_interface);
bool destroy(r_module_properties *lib_interface);

bool pre_frame(r_module_properties *lib_interface, float delta_time);
bool update(r_module_properties *lib_interface, float delta_time);
bool ui_update(r_module_properties *lib_interface, float delta_time);
bool post_frame(r_module_properties *lib_interface, float delta_time);

bool on_unload(r_module_properties *lib_interface);
bool on_reload(r_module_properties *lib_interface);



#ifdef __cplusplus
}
#endif

#endif // BASIC_H