#pragma once

#include "defs.h"

enum error_color {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
};

struct error_message {
    i32 const line_start;
    i32 const column_start;
    struct string message;
    struct string path;
    struct string line_buffer;
    struct error_message* next;
};

void print_error_message(
    struct error_message* error_message, enum error_color color
);

void error_message_add_note(
    struct error_message* parent, struct error_message* note
);

struct error_message error_message_create_with_line(
    struct string path, i32 line, i32 column, struct string source,
    i32* line_offsets, struct string message
);
