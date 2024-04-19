#pragma once

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// __attribute__((format(printf, 1, 2)))
[[__noreturn__]] static void vix_panic(char* format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
    abort();
}

[[__noreturn__]] static void vix_unreachable(void) {
    vix_panic("unreachable");
}
