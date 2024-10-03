#pragma once

#include "defs.h"

#include <execinfo.h>
#include <stdnoreturn.h>
#include <unistd.h>

extern char const** sources;

enum exit_status {
    EXIT_USER     = 1,
    EXIT_LEX      = 2,
    EXIT_PARSE    = 3,
    EXIT_VALIDATE = 4,
    EXIT_ABNORMAL = 255
};

// __attribute__((format(printf, 1, 2)))
noreturn void vix_panic(char* format, ...);

noreturn void vix_unreachable(void);

struct location {
    i32 file;
    i32 line_number;
    i32 column_number;
};

void error_line(struct location location);

struct dynamic_array_header {
    i64 length;
    i64 capacity;
    i64 stride;
};

static i64 const header_size = sizeof(struct dynamic_array_header);

// static void
// _dynamic_array_set_length(void* array, i64 length) {
//     struct dynamic_array_header* header
//         = (struct dynamic_array_header*) ((u8*) array - header_size);
//     header->length = length;
// }
//
// static i64
// dynamic_array_length(void* array) {
//     struct dynamic_array_header* header
//         = (struct dynamic_array_header*) ((u8*) array - header_size);
//     return header->length;
// }
//
// static void*
// _dynamic_array_create(i64 length, i64 stride) {
//     i64 array_size                      = length * stride;
//     void* new_array                     = calloc(1, header_size + array_size);
//     struct dynamic_array_header* header = new_array;
//     header->capacity                    = length;
//     header->length                      = length;
//     header->stride                      = stride;
//     return (void*) ((u8*) new_array + header_size);
// }
//
// #define dynamic_array_create(type) _dynamic_array_create(1, sizeof(type))
//
// static void
// dynamic_array_destroy(void* array) {
//     if (array) {
//         free(array);
//     }
// }
//
// static void*
// dynamic_array_resize(void* array) {
//     if (array) {
//         struct dynamic_array_header* header
//             = (struct dynamic_array_header*) ((u8*) array - header_size);
//         array = realloc(header, header_size + 2 * header->capacity);
//         header = (struct dynamic_array_header*) ((u8*) array - header_size);
//         header->capacity *= 2;
//         return array;
//     }
//     return nullptr;
// }
//
// static void*
// _dynamic_array_push(void* array, void const* value_ptr) {
//     struct dynamic_array_header* header
//         = (struct dynamic_array_header*) ((u8*) array - header_size);
//     if (header->length >= header->capacity) {
//         array = dynamic_array_resize(array);
//     }
//     header = (struct dynamic_array_header*) ((u8*) array - header_size);
//
//     i64 address = (i64) array;
//     address += header->length * header->stride;
//     memcpy((void*) address, value_ptr, header->stride);
//     _dynamic_array_set_length(array, header->length + 1);
//     return array;
// }

#define dynamic_array_push(array, value)                      \
    {                                                         \
        typeof(value) tmp = value;                            \
        array             = _dynamic_array_push(array, &tmp); \
    }

// static void
// dynamic_array_pop(void* array, void* dest) {
//     struct dynamic_array_header* header
//         = (struct dynamic_array_header*) ((u8*) array - header_size);
//     i64 address = (i64) array;
//     address += (header->length - 1) * header->stride;
//     memcpy(dest, (void*) address, header->stride);
//     _dynamic_array_set_length(array, header->length - 1);
// }
