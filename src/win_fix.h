#ifndef WIN_FIX_H
#define WIN_FIX_H
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static inline int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (size < 0) return -1;
    *strp = (char *)malloc(size + 1);
    if (!*strp) return -1;
    va_start(args, fmt);
    size = vsprintf(*strp, fmt, args);
    va_end(args);
    return size;
}
#endif