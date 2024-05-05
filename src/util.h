#pragma once

#include "defs.h"

#include <execinfo.h>
#include <stdnoreturn.h>
#include <unistd.h>

extern char const** sources;

enum exit_status_e {
    EXIT_USER     = 1,
    EXIT_LEX      = 2,
    EXIT_PARSE    = 3,
    EXIT_VALIDATE = 4,
    EXIT_ABNORMAL = 255
};

// __attribute__((format(printf, 1, 2)))
noreturn void vix_panic(char* format, ...);

noreturn void vix_unreachable(void);

struct Array {
    void* data;
    i64 length;
    i64 capacity;
};

void* array_reallocate(void* buffer, i64 length, i64 element_size);
void* xarray_reallocate(void* buffer, i64 length, i64 element_size);
void* xmalloc(i64 length);

void* array_add(struct Array array[static 1], i64 length);
void array_add_ptr(struct Array array[static 1], void* value);
void array_add_buffer(
    struct Array array[static 1], void const* buffer, i64 length
);
#define array_for_each(a, n)                                                 \
    for (void*(n) = (a)->data; m != (void*) ((u8*) (a)->data + (a)->length); \
         ++n)

struct location_t {
    i32 file;
    i32 line_number;
    i32 column_number;
};

void error_line(struct location_t location);
