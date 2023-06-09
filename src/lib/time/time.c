#include <time.h>

#include "time/time.h"

static struct timespec start;

void r_time_init() {
    clock_gettime(CLOCK_REALTIME, &start);
}

float _get_time_ms() {

    // get the current time in milliseconds and subtract the start time so that it 
    // is always relative to the start of the program
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    float sec = (current_time.tv_sec - start.tv_sec) * 1000.f;
    float ns = (current_time.tv_nsec - start.tv_nsec) / 1000000.f;
    return (sec + ns);
}
