#ifndef _FILETRACKER_H_
#define _FILETRACKER_H_

#include "module/interface.h"

typedef struct r_filetracker r_filetracker;

#define CHECK_FREQUENCY_S 1.0f

r_filetracker * r_filetracker_create();
void r_filetracker_destroy(r_filetracker *filetracker);
void r_filetracker_add_module(r_filetracker *filetracker, r_module_interface *module);
void r_filetracker_remove_module(r_filetracker *filetracker, const char *lib_path);
void r_filetracker_check(r_filetracker *filetracker, float delta_time);



#endif