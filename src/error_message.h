#pragma once

#include "allocator.h"
#include "string.h"

typedef enum {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
} ErrorColor;

typedef struct ErrorMessage ErrorMessage;
struct ErrorMessage {
    i64 const line_start;
    i64 const column_start;
    Str const message;
    Str const path;
    Str line_buffer;
    ErrorMessage** notes;
};

void print_error_message(
    ErrorMessage error_message[static 1], ErrorColor color
);

void error_message_add_note(
    ErrorMessage parent[static 1], ErrorMessage note[static 1]
);

ErrorMessage error_message_create_with_offset(
    Allocator allocator[static 1], Str path, i64 line, i64 column,
    i64 offset, Str source, Str message
);

ErrorMessage error_message_create_with_line(
    Allocator allocator[static 1], Str path, i64 line, i64 column,
    Str source, i64* line_offsets, Str message
);
