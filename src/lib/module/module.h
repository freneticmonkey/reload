#ifndef _MODULE_LIFECYCLE_H_
#define _MODULE_LIFECYCLE_H_

// r_module_lifecycle maintains a list of hotloadable modules which 
// meet the r_module_interface

#include <stdbool.h>
#include <stdint.h>

#include "module/interface.h"

typedef struct r_module_lifecycle r_module_lifecycle;

r_module_lifecycle * r_module_lifecycle_create();
void r_module_lifecycle_destroy(r_module_lifecycle *lifecycle);

r_module_interface * r_module_lifecycle_register(r_module_lifecycle *lifecycle, r_module_properties properties);
void r_module_lifecycle_unregister(r_module_lifecycle *lifecycle, r_module_interface *interface);

void r_module_lifecycle_update(r_module_lifecycle *lifecycle, float delta_ms);

// void r_module_lifecyle_check_for_reload(r_module_lifecycle *lifecycle);


#endif