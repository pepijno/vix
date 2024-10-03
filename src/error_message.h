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
    char const* message;
    char const* path;
    char* line_buffer;
    struct error_message* next;
};

void print_error_message(
    struct error_message error_message[static 1], enum error_color color
);

void error_message_add_note(
    struct error_message parent[static 1], struct error_message note[static 1]
);

struct error_message error_message_create_with_line(
    char* path, i32 line, i32 column, char* source, i32* line_offsets,
    char* message
);
