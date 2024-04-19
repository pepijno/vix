#include "error_message.h"

#include "array.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

typedef enum {
    ERROR_TYPE_ERROR,
    ERROR_TYPE_NOTE
} error_type_e;

typedef enum {
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE,
    TERM_COLOR_RESET
} term_color_e;

#define VT_RED   "\x1b[31;1m"
#define VT_GREEN "\x1b[32;1m"
#define VT_CYAN  "\x1b[36;1m"
#define VT_WHITE "\x1b[37;1m"
#define VT_RESET "\x1b[0m"

static void set_color_posix(term_color_e const color) {
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

static void print_error_message_type(
    error_message_t const error_message[static const 1],
    error_color_e const color, error_type_e const error_type
) {
    str_t const path  = error_message->path;
    size_t const line = error_message->line_start + 1;
    size_t const col  = error_message->column_start + 1;
    str_t const text  = error_message->message;

    bool const is_tty = isatty(STDERR_FILENO) != 0;
    if (color == ERROR_COLOR_ON || (color == ERROR_COLOR_AUTO && is_tty)) {
        if (error_type == ERROR_TYPE_ERROR) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, str_fmt ":%zu:%zu: ", str_args(path), line, col);
            set_color_posix(TERM_COLOR_RED);
            fprintf(stderr, "error: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " " str_fmt, str_args(text));
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else if (error_type == ERROR_TYPE_NOTE) {
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, str_fmt ":%zu:%zu: ", str_args(path), line, col);
            set_color_posix(TERM_COLOR_CYAN);
            fprintf(stderr, "note: ");
            set_color_posix(TERM_COLOR_WHITE);
            fprintf(stderr, " " str_fmt, str_args(text));
            set_color_posix(TERM_COLOR_RESET);
            fprintf(stderr, "\n");
        } else {
            vix_unreachable();
        }

        fprintf(stderr, str_fmt "\n", str_args(error_message->line_buffer));
        for (size_t i = 0; i < error_message->column_start; ++i) {
            fprintf(stderr, " ");
        }
        set_color_posix(TERM_COLOR_GREEN);
        fprintf(stderr, "^");
        set_color_posix(TERM_COLOR_RESET);
        fprintf(stderr, "\n");
    } else {
        if (error_type == ERROR_TYPE_ERROR) {
            fprintf(
                stderr, str_fmt ":%zu:%zu: error: " str_fmt "\n",
                str_args(path), line, col, str_args(text)
            );
        } else if (error_type == ERROR_TYPE_NOTE) {
            fprintf(
                stderr, str_fmt ":%zu:%zu: note: " str_fmt "\n", str_args(path),
                line, col, str_args(text)
            );
        } else {
            vix_unreachable();
        }
    }

    for (size_t i = 0; i < array_length_unsigned(error_message->notes); ++i) {
        error_message_t const* const note = error_message->notes[i];
        print_error_message_type(note, color, ERROR_TYPE_NOTE);
    }
}

void print_error_message(
    error_message_t const error_message[static const 1],
    error_color_e const color
) {
    print_error_message_type(error_message, color, ERROR_TYPE_ERROR);
}

void error_message_add_note(
    error_message_t parent[static const 1], error_message_t note[static const 1]
) {
    array_push(parent->notes, note);
}

error_message_t error_message_create_with_offset(
    allocator_t allocator[static const 1], str_t const path, size_t const line,
    size_t const column, size_t const offset, str_t const source,
    str_t const message
) {
    error_message_t error_message = {
        .path         = path,
        .line_start   = line,
        .column_start = column,
        .message      = message,
    };
    error_message.notes = array(error_message_t*, allocator);

    size_t line_start_offset = offset;
    while (true) {
        if (line_start_offset == 0) {
            break;
        }

        line_start_offset -= 1;

        if (str_at(source, line_start_offset) == '\n') {
            line_start_offset += 1;
            break;
        }
    }

    size_t line_end_offset = offset;
    while (source.length > line_start_offset &&
           str_at(source, line_end_offset) != '\n') {
        line_end_offset += 1;
    }

    error_message.line_buffer = str_new_length(
        source.data + line_start_offset, line_end_offset - line_start_offset
    );

    return error_message;
}

error_message_t error_message_create_with_line(
    allocator_t allocator[static const 1], str_t const path, size_t const line,
    size_t const column, str_t const source, size_t const* const line_offsets,
    str_t const message
) {
    error_message_t error_message = {
        .path         = path,
        .line_start   = line,
        .column_start = column,
        .message      = message,
    };
    error_message.notes = array(error_message_t*, allocator);

    size_t const line_start_offset = line_offsets[line];
    size_t const end_line          = line + 1;
    size_t const line_end_offset =
        (end_line >= array_header(line_offsets)->length)
            ? source.length
            : line_offsets[line + 1];
    size_t const length = (line_end_offset + 1 > line_start_offset)
                            ? (line_end_offset - line_start_offset - 1)
                            : 0;

    error_message.line_buffer =
        str_new_length(source.data + line_start_offset, length);

    return error_message;
}
