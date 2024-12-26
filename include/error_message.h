#pragma once

#include "defs.h"
#include "str.h"

enum error_color {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
};

struct error_message {
    usize const line_start;
    usize const column_start;
    struct string message;
    struct string path;
    struct string line_buffer;
    struct error_message* next;
};

void print_error_message(
    struct error_message error_message[static const 1],
    enum error_color const color
);

void error_message_add_note(
    struct error_message parent[static const 1],
    struct error_message note[static const 1]
);

struct error_message error_message_create_with_line(
    struct string const path, usize const line, usize const column,
    struct string const source, usize* const line_offsets,
    struct string const message
);
