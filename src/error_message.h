#pragma once

#include "defs.h"

typedef enum {
    ERROR_COLOR_AUTO,
    ERROR_COLOR_OFF,
    ERROR_COLOR_ON
} ErrorColor;

typedef struct ErrorMessage ErrorMessage;
struct ErrorMessage {
    i32 const line_start;
    i32 const column_start;
    char const* message;
    char const* path;
    char* line_buffer;
    ErrorMessage* next;
};

void print_error_message(
    ErrorMessage error_message[static 1], ErrorColor color
);

void error_message_add_note(
    ErrorMessage parent[static 1], ErrorMessage note[static 1]
);

ErrorMessage error_message_create_with_line(
    char* path, i32 line, i32 column, char* source, i32* line_offsets,
    char* message
);
