#include "error_message.h"

#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef enum {
    ERROR_TYPE_ERROR,
    ERROR_TYPE_NOTE
} ErrorType;

typedef enum {
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE,
    TERM_COLOR_RESET
} TermColor;

#define VT_RED   "\x1b[31;1m"
#define VT_GREEN "\x1b[32;1m"
#define VT_CYAN  "\x1b[36;1m"
#define VT_WHITE "\x1b[37;1m"
#define VT_RESET "\x1b[0m"

static void
set_color_posix(TermColor color) {
    switch (color) {
        case TERM_COLOR_RED:
            fprintf(stderr, VT_RED);
            break;
        case TERM_COLOR_GREEN:
            fprintf(stderr, VT_GREEN);
            break;
        case TERM_COLOR_CYAN:
            fprintf(stderr, VT_CYAN);
            break;
        case TERM_COLOR_WHITE:
            fprintf(stderr, VT_WHITE);
            break;
        case TERM_COLOR_RESET:
            fprintf(stderr, VT_RESET);
            break;
    }
}

static void
print_error_message_type(
    ErrorMessage error_message[static 1], ErrorColor color, ErrorType error_type
) {
    char const* path = error_message->path;
    i32 line         = error_message->line_start + 1;
    i32 col          = error_message->column_start + 1;
    char const* text = error_message->message;

    bool is_tty = isatty(STDERR_FILENO) != 0;
    if (color == ERROR_COLOR_ON || (color == ERROR_COLOR_AUTO && is_tty)) {
        if (error_type == ERROR_TYPE_ERROR) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, "%s:%d:%d: ", path, line, col);
            set_color_posix(TERM_COLOR_RED);
            fprintf(stderr, "error: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " %s", text);
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else if (error_type == ERROR_TYPE_NOTE) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, "%s:%d:%d: ", path, line, col);
            set_color_posix(TERM_COLOR_CYAN);
            fprintf(stderr, "note: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " %s", text);
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else {
            vix_unreachable();
        }

        fprintf(stderr, "%s\n", error_message->line_buffer);
        for (i64 i = 0; i < error_message->column_start; ++i) {
            fprintf(stderr, " ");
        }
        set_color_posix(TERM_COLOR_GREEN);
        fprintf(stderr, "^");
        set_color_posix(TERM_COLOR_RESET);
        fprintf(stderr, "\n");
    } else {
        if (error_type == ERROR_TYPE_ERROR) {
            fprintf(stderr, "%s:%d:%d: error: %s\n", path, line, col, text);
        } else if (error_type == ERROR_TYPE_NOTE) {
            fprintf(stderr, "%s:%d:%d: note: %s\n", path, line, col, text);
        } else {
            vix_unreachable();
        }
    }

    ErrorMessage* next = error_message;
    while (next) {
        print_error_message_type(next, color, ERROR_TYPE_NOTE);
        next = next->next;
    }
}

void
print_error_message(ErrorMessage error_message[static 1], ErrorColor color) {
    print_error_message_type(error_message, color, ERROR_TYPE_ERROR);
}

void
error_message_add_note(
    ErrorMessage parent[static 1], ErrorMessage note[static 1]
) {
    ErrorMessage** next = &parent->next;
    while (next) {
        next = &(*next)->next;
    }
    *next = note;
}

ErrorMessage
error_message_create_with_line(
    char* path, i32 line, i32 column, char* source, i32* line_offsets,
    char* message
) {
    (void) source;
    (void) line_offsets;
    ErrorMessage error_message = {
        .path         = path,
        .line_start   = line,
        .column_start = column,
        .message      = message,
        .next         = nullptr,
    };

    // i32 line_start_offset = line_offsets[line];
    // // i32 end_line          = line + 1;
    // i32 line_end_offset = line_offsets[line + 1];
    // i32 length          = (line_end_offset + 1 > line_start_offset)
    //                         ? (line_end_offset - line_start_offset - 1)
    //                         : 0;

    // memcpy(error_message.line_buffer, source + line_start_offset, length);

    return error_message;
}
