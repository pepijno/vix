#include "util.h"

#include <stdarg.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char const** sources;

noreturn void
vix_panic(char* format, ...) {
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__);
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
    abort();
}

noreturn void
vix_unreachable(void) {
    vix_panic("unreachable");
}

static i32
getline(char** lineptr, i32* n, FILE* stream) {
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
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size   = size + 128;
            bufptr = realloc(bufptr, size);
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
error_line(struct location location) {
    char const* path = sources[location.file];
    struct stat filestat;
    if (stat((char*) path, &filestat) == -1 || !S_ISREG(filestat.st_mode)) {
        return;
    }

    FILE* src = fopen((char*) path, "r");
    if (!src) {
        return;
    }
    char* line = nullptr;
    i32 size   = 0;
    i32 length = 0;
    i32 n      = 0;
    while (n < location.line_number) {
        if ((length = getline(&line, &size, src)) == -1) {
            fclose(src);
            free(line);
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

        free(line);
    }

    fclose(src);
}
