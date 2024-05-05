#include "util.h"

#include <assert.h>
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

void*
array_reallocate(void* buffer, i64 length, i64 element_size) {
    return realloc(buffer, length * element_size);
}

void*
xarray_reallocate(void* buffer, i64 length, i64 element_size) {
    buffer = realloc(buffer, length * element_size);
    if (buffer == nullptr && length != 0 && element_size != 0) {
        vix_panic("array_reallocate:");
    }
    return buffer;
}

void*
xmalloc(i64 length) {
    void* buffer = malloc(length);
    if (buffer == nullptr && length != 0) {
        vix_panic("malloc:");
    }
    return buffer;
}

void*
array_add(struct Array array[static 1], i64 length) {
    if (array->capacity - array->length < length) {
        do {
            array->capacity = array->capacity != 0 ? array->capacity * 2 : 8;
        } while (array->capacity - array->length < length);
        array->data = realloc(array->data, array->capacity);
        if (array->data == nullptr) {
            vix_panic("realloc");
        }
    }
    void* v = (u8*) array->data + array->length;
    array->length += length;
    return v;
}

void
array_add_ptr(struct Array array[static 1], void* value) {
    *(void**) array_add(array, sizeof(value)) = value;
}

void
array_add_buffer(struct Array array[static 1], void const* buffer, i64 length) {
    memcpy(array_add(array, length), buffer, length);
}

void*
array_last(struct Array array[static 1], i64 length) {
    if (array->length == 0) {
        return nullptr;
    }
    assert(length <= array->length);
    return (u8*) array->data + array->length - length;
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
error_line(struct location_t location) {
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
