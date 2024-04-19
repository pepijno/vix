#pragma once

#include "allocator.h"
#include "string.h"

typedef enum {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
} error_color_e;

typedef struct error_message_t error_message_t;
struct error_message_t {
    size_t const line_start;
    size_t const column_start;
    str_t const message;
    str_t const path;
    str_t line_buffer;
    error_message_t** notes;
};

void print_error_message(
    error_message_t const error_message[static const 1],
    error_color_e const color
);

void error_message_add_note(
    error_message_t parent[static const 1], error_message_t note[static const 1]
);

error_message_t error_message_create_with_offset(
    allocator_t allocator[static const 1], str_t const path, size_t const line,
    size_t const column, size_t const offset, str_t const source,
    str_t const message
);

error_message_t error_message_create_with_line(
    allocator_t allocator[static const 1], str_t const path, size_t const line,
    size_t const column, str_t const source, size_t const* const line_offsets,
    str_t const message
);
