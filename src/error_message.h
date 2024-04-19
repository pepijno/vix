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
    error_message_t error_message[static 1], error_color_e color
);

void error_message_add_note(
    error_message_t parent[static 1], error_message_t note[static 1]
);

error_message_t error_message_create_with_offset(
    allocator_t allocator[static 1], str_t path, size_t line, size_t column,
    size_t offset, str_t source, str_t message
);

error_message_t error_message_create_with_line(
    allocator_t allocator[static 1], str_t path, size_t line, size_t column,
    str_t source, size_t* line_offsets, str_t message
);
