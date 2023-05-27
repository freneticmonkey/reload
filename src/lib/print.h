#ifndef LIB_PRINT_H
#define LIB_PRINT_H

// enable print capture
char * l_print_enable_capture();

// enable print suppression - used for unit tests
void l_print_enable_suppress();

// custom print function that allows for optional
// capture of output
int l_printf(const char *format, ...);

#endif