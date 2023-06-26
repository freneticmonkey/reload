#ifndef _MODULE_HELPER_H_
#define _MODULE_HELPER_H_


void r_module_create();
void r_module_destroy();

void r_module_add(const char *module_name);

void r_module_pre_frame(float delta_time);
void r_module_update(float delta_time);
void r_module_ui_update(float delta_time);
void r_module_post_frame(float delta_time);

#endif