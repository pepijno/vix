#include "util.h"

#include "allocator.h"

#include <stdarg.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct string* sources;

noreturn void
vix_panic(struct string format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
    vfprintf(stderr, format.data, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
    abort();
}

noreturn void
vix_unreachable(void) {
    vix_panic(from_cstr("unreachable"));
}

static i32
getline(struct arena* arena, char** lineptr, i32* n, FILE* stream) {
    char* bufptr = NULL;
    char* p      = bufptr;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr   = *lineptr;
    i32 size = *n;

    i32 c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = arena_allocate(arena, 128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            bufptr = arena_resize(arena, bufptr, size, size + 128);
            size   = size + 128;
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++     = '\0';
    *lineptr = bufptr;
    *n       = size;

    return p - bufptr - 1;
}

void
error_line(struct arena* arena, struct location location) {
    struct string path = sources[location.file];
    struct stat filestat;
    if (stat(path.data, &filestat) == -1 || !S_ISREG(filestat.st_mode)) {
        return;
    }

    FILE* src = fopen(path.data, "r");
    if (!src) {
        return;
    }
    char* line = nullptr;
    i32 size   = 0;
    i32 length = 0;
    i32 n      = 0;
    while (n < location.line_number) {
        if ((length = getline(arena, &line, &size, src)) == -1) {
            fclose(src);
            return;
        }
        n += 1;
    }

    if (line != nullptr) {
        fprintf(stderr, "\n");
        i32 line_number_width = fprintf(stderr, "%d", location.line_number);
        fprintf(
            stderr, " |\t%s%s%*c |\n%*c", line,
            line[length - 1] == '\n' ? "" : "\n", line_number_width, ' ',
            location.column_number - 1, ' '
        );
    }

    fclose(src);
}
