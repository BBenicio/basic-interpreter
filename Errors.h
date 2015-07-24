#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

inline void vError(const char* fmt, va_list& args) {
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
}

void error(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vError(fmt, args);
    va_end(args);
}

void fatalError(int exitCode, const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vError(fmt, args);
    va_end(args);

    exit(exitCode);
}

#endif // ERRORS_H_INCLUDED
