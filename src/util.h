#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

[[__noreturn__]] static void vix_panic(char const* const format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
    abort();
}

[[__noreturn__]] static void vix_unreachable(void) {
    vix_panic("unreachable");
}
