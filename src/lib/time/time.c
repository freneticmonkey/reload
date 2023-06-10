#include <time.h>
#include <unistd.h>

#include "time/time.h"

typedef struct _r_time {
    struct timespec start;
    float  fps;
    float  frame_target;
    float  remaining;
    float  last;
} _r_time;

static _r_time _time;

float _get_time_ms() {
    // get the current time in milliseconds and subtract the start time so that it 
    // is always relative to the start of the program
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    float sec = (current_time.tv_sec - _time.start.tv_sec) * 1000.f;
    float ns = (current_time.tv_nsec - _time.start.tv_nsec) / 1000000.f;
    return (sec + ns);
}

void r_time_init(float fps)  {
    _time = (_r_time){
        .start = {0},
        .fps = fps,
        .frame_target = 1000.f / fps,
        .remaining = 0.f,
    };

    // initialise the start time
    clock_gettime(CLOCK_REALTIME, &_time.start);
    _time.last = _get_time_ms();
}

float r_time_get_delta() {
    float now = _get_time_ms();
    float delta_time = now - _time.last;
    _time.last = now;
    _time.remaining = _time.frame_target - delta_time;
    return delta_time;
}

void r_time_sleep_remaining() {
    if (_time.remaining > 0.f) {
        usleep(_time.remaining * 1000.f);
    }
}
