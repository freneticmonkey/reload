#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "lib/print.h"
#include "lib/memory.h"

// This is to capture the output of the print function
// so that we can validate output in unit tests
#define PRINT_BUFFER_SIZE 1024 * 1024

bool _suppress_print = false;
char * _print_buffer = NULL;
size_t position = 0;

// enable print capture
char * l_print_enable_capture() {
    _print_buffer = malloc(PRINT_BUFFER_SIZE);
    if (_print_buffer != NULL)
        memset(_print_buffer, 0, PRINT_BUFFER_SIZE);    
    return _print_buffer;
}

void l_print_enable_suppress() {
    _suppress_print = true;
}

// disable print capture
void l_print_disable_capture() {
    free(_print_buffer);
    _print_buffer = NULL;
}

// custom print function that allows for optional
// capture of output
int l_printf(const char *format, ...) {
    va_list args;
    int ret = 0;
    

    // FIXME: This needs to write to a buffer and then the target pointer needs to be 
    // moved by the amount written in order for the buffer to not be overwritten
    if (_print_buffer != NULL) {
        va_start(args, format);
        ret = vsnprintf(_print_buffer, PRINT_BUFFER_SIZE, format, args);
        position += ret;
        _print_buffer += ret;
        va_end(args);

        if (position >= PRINT_BUFFER_SIZE) {
            printf("Print buffer overflowed.  Increase PRINT_BUFFER_SIZE in print.c\n");
            l_print_disable_capture();
        }
    }

    if (!_suppress_print) {
        va_start(args, format);
        ret = vprintf(format, args);
        va_end(args);
    }

    return ret;
}