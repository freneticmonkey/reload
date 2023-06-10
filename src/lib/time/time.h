#ifndef _R_TIME_H_
#define _R_TIME_H_

#include <stdint.h>

// initialise time
void r_time_init(float fps);

// loop helpers
float r_time_get_delta();
float r_time_sleep_remaining();

#endif