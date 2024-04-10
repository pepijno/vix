#ifndef ERROR_MESSAGE_H
#define ERROR_MESSAGE_H

#include "allocator.h"

enum error_color_t {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
};

struct error_message_t {
    size_t const line_start;
    size_t const column_start;
    char const* const message;
    char const* const path;
    char* line_buffer;
    struct error_message_t** notes;
};

void print_error_message(
    struct allocator_t* allocator,
    struct error_message_t const error_message[static const 1],
    enum error_color_t const color
);

void error_message_add_note(
    struct error_message_t parent[static const 1],
    struct error_message_t note[static const 1]
);

struct error_message_t error_message_create_with_offset(
    struct allocator_t allocator[static const 1], char const* const path,
    size_t const line, size_t const column, size_t const offset,
    char const* const source, char const* const message
);

struct error_message_t error_message_create_with_line(
    struct allocator_t allocator[static const 1], char const* const path,
    size_t const line, size_t const column, char const* const source,
    size_t const* const line_offsets, char const* const message
);

#endif
