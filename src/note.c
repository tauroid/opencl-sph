#include <stdio.h>
#include <stdarg.h>
#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

static unsigned int _loglevel = 1;

void set_log_level(unsigned int loglevel) {
    _loglevel = loglevel;
}

void note(unsigned int level, const char * fmt, ...) {
    if (level < _loglevel) return;

#ifndef MATLAB_MEX_FILE
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#else
    char * buffer;
    size_t size;
    FILE * stream = open_memstream(&buffer, &size);

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    fclose(stream);
    mexPrintf("%s", buffer);
    free(buffer);
#endif
}
