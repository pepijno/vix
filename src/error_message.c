#include "error_message.h"

#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

enum error_type {
    ERROR_TYPE_ERROR,
    ERROR_TYPE_NOTE
};

enum term_color {
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE,
    TERM_COLOR_RESET
};

#define VT_RED   "\x1b[31;1m"
#define VT_GREEN "\x1b[32;1m"
#define VT_CYAN  "\x1b[36;1m"
#define VT_WHITE "\x1b[37;1m"
#define VT_RESET "\x1b[0m"

static void
set_color_posix(enum term_color const color) {
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
    struct error_message error_message[static const 1],
    enum error_color const color, enum error_type const error_type
) {
    struct string path = error_message->path;
    usize line         = error_message->line_start + 1;
    usize col          = error_message->column_start + 1;
    struct string text = error_message->message;

    bool is_tty = isatty(STDERR_FILENO) != 0;
    if (color == ERROR_COLOR_ON || (color == ERROR_COLOR_AUTO && is_tty)) {
        if (error_type == ERROR_TYPE_ERROR) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, STR_FMT ":%lu:%lu: ", STR_ARG(path), line, col);
            set_color_posix(TERM_COLOR_RED);
            fprintf(stderr, "error: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " " STR_FMT, STR_ARG(text));
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else if (error_type == ERROR_TYPE_NOTE) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, STR_FMT ":%lu:%lu: ", STR_ARG(path), line, col);
            set_color_posix(TERM_COLOR_CYAN);
            fprintf(stderr, "note: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " " STR_FMT, STR_ARG(text));
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else {
            vix_unreachable();
        }

        fprintf(stderr, STR_FMT "\n", STR_ARG(error_message->line_buffer));
        for (usize i = 0; i < error_message->column_start; i += 1) {
            fprintf(stderr, " ");
        }
        set_color_posix(TERM_COLOR_GREEN);
        fprintf(stderr, "^");
        set_color_posix(TERM_COLOR_RESET);
        fprintf(stderr, "\n");
    } else {
        if (error_type == ERROR_TYPE_ERROR) {
            fprintf(
                stderr, STR_FMT ":%lu:%lu: error: " STR_FMT "\n", STR_ARG(path),
                line, col, STR_ARG(text)
            );
        } else if (error_type == ERROR_TYPE_NOTE) {
            fprintf(
                stderr, STR_FMT ":%lu:%lu: note: " STR_FMT "\n", STR_ARG(path),
                line, col, STR_ARG(text)
            );
        } else {
            vix_unreachable();
        }
    }

    struct error_message* next = error_message;
    while (next) {
        print_error_message_type(next, color, ERROR_TYPE_NOTE);
        next = next->next;
    }
}

void
print_error_message(
    struct error_message error_message[static const 1],
    enum error_color const color
) {
    print_error_message_type(error_message, color, ERROR_TYPE_ERROR);
}

void
error_message_add_note(
    struct error_message parent[static const 1],
    struct error_message note[static const 1]
) {
    struct error_message** next = &parent->next;
    while (next) {
        next = &(*next)->next;
    }
    *next = note;
}

struct error_message
error_message_create_with_line(
    struct string const path, usize const line, usize const column,
    struct string const source, usize* const line_offsets,
    struct string const message
) {
    (void) source;
    (void) line_offsets;
    struct error_message error_message = {
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
