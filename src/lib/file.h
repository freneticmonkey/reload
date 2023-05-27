#ifndef LIB_FILE_H
#define LIB_FILE_H

#include <stdbool.h>

#include "vm.h"

int l_run_file(vm_config_t *config);

char* l_read_file(const char* path);
bool  l_file_exists(const char * path);
bool  l_file_delete(const char * path);

#endif