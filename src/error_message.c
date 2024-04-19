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

static void set_color_posix(term_color_e color) {
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
    error_message_t error_message[static 1],
    error_color_e color, error_type_e error_type
) {
    str_t path  = error_message->path;
    size_t line = error_message->line_start + 1;
    size_t col  = error_message->column_start + 1;
    str_t text  = error_message->message;

    bool is_tty = isatty(STDERR_FILENO) != 0;
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
        error_message_t * note = error_message->notes[i];
        print_error_message_type(note, color, ERROR_TYPE_NOTE);
    }
}

void print_error_message(
    error_message_t error_message[static 1],
    error_color_e color
) {
    print_error_message_type(error_message, color, ERROR_TYPE_ERROR);
}

void error_message_add_note(
    error_message_t parent[static 1], error_message_t note[static 1]
) {
    array_push(parent->notes, note);
}

error_message_t error_message_create_with_offset(
    allocator_t allocator[static 1], str_t path, size_t line,
    size_t column, size_t offset, str_t source,
    str_t message
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
    allocator_t allocator[static 1], str_t path, size_t line,
    size_t column, str_t source, size_t * line_offsets,
    str_t message
) {
    error_message_t error_message = {
        .path         = path,
        .line_start   = line,
        .column_start = column,
        .message      = message,
    };
    error_message.notes = array(error_message_t*, allocator);

    size_t line_start_offset = line_offsets[line];
    size_t end_line          = line + 1;
    size_t line_end_offset =
        (end_line >= array_header(line_offsets)->length)
            ? source.length
            : line_offsets[line + 1];
    size_t length = (line_end_offset + 1 > line_start_offset)
                            ? (line_end_offset - line_start_offset - 1)
                            : 0;

    error_message.line_buffer =
        str_new_length(source.data + line_start_offset, length);

    return error_message;
}
