#include "lexer.h"

#include "allocator.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct string const token_names[]
    = { [TOKEN_NONE]         = {
        .buffer = "TOKEN_NONE",
        .length = 10,
    },
    [TOKEN_NAME]         = {
        .buffer = "TOKEN_NAME",
        .length = 10,
    },
    [TOKEN_ASSIGN]         = {
        .buffer = "TOKEN_ASSIGN",
        .length = 12,
    },
    [TOKEN_DOT]         = {
        .buffer = "TOKEN_DOT",
        .length = 9,
    },
    [TOKEN_DOT_DOT_DOT]         = {
        .buffer = "TOKEN_DOT_DOT_DOT",
        .length = 17,
    },
    [TOKEN_COMMA]         = {
        .buffer = "TOKEN_COMMA",
        .length = 11,
    },
    [TOKEN_CHAR]         = {
        .buffer = "TOKEN_CHAR",
        .length = 10,
    },
    [TOKEN_STRING]         = {
        .buffer = "TOKEN_STRING",
        .length = 12,
    },
    [TOKEN_OPEN_BRACE]         = {
        .buffer = "TOKEN_OPEN_BRACE",
        .length = 16,
    },
    [TOKEN_CLOSE_BRACE]         = {
        .buffer = "TOKEN_CLOSE_BRACE",
        .length = 17,
    },
    [TOKEN_OPEN_PAREN]         = {
        .buffer = "TOKEN_OPEN_PAREN",
        .length = 16,
    },
    [TOKEN_CLOSE_PAREN]         = {
        .buffer = "TOKEN_CLOSE_PAREN",
        .length = 17,
    },
    [TOKEN_GREATER_THAN]         = {
        .buffer = "TOKEN_GREATER_THAN",
        .length = 18,
    },
    [TOKEN_SEMICOLON]         = {
        .buffer = "TOKEN_SEMICOLON",
        .length = 15,
    },
    [TOKEN_INTEGER]         = {
        .buffer = "TOKEN_INTEGER",
        .length = 13,
    },
    [TOKEN_EOF]         = {
        .buffer = "TOKEN_EOF",
        .length = 9,
    }
};

static noreturn void
error(struct arena* arena, struct location location, char const* format, ...) {
    fprintf(
        stderr, "%s:%d:%d: syntax error: ", "vix", location.line_number,
        location.column_number
    );

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fprintf(stderr, "\n");
    error_line(arena, location);
    exit(EXIT_LEX);
}

struct lexer
lexer_new(struct arena* arena, FILE* f, i32 file_id) {
    return (struct lexer){
        .in          = f,
        .buffer_size = 256,
        .buffer      = arena_allocate(arena, 256),
        .un =
            (struct token){
                             .type = TOKEN_NONE,
                             },
        .location =
            (struct location){
                             .line_number   = 1,
                             .column_number = 0,
                             .file          = file_id,
                             },
        .c =
            {
                             (struct codepoint){.ok = false},
                             (struct codepoint){.ok = false},
                             },
    };
}

static void
lexer_clear_buffer(struct lexer* lexer) {
    lexer->buffer_length = 0;
    lexer->buffer[0]     = 0;
}

void
lexer_finish(struct lexer* lexer) {
    fclose(lexer->in);
}

static void
append_buffer(
    struct arena* arena, struct lexer* lexer, struct utf8char utf8_char
) {
    if (lexer->buffer_length + utf8_char.length >= lexer->buffer_size) {
        i32 old_buffer_size = lexer->buffer_size;
        do {
            lexer->buffer_size *= 2;
        } while (lexer->buffer_length + utf8_char.length >= lexer->buffer_size);
        lexer->buffer = arena_resize(
            arena, lexer->buffer, old_buffer_size, lexer->buffer_size
        );
    }
    memcpy(
        lexer->buffer + lexer->buffer_length, &utf8_char.characters,
        utf8_char.length
    );
    lexer->buffer_length += utf8_char.length;
    lexer->buffer[lexer->buffer_length] = '\0';
}

static void
update_line_number(struct location* location, struct codepoint cp) {
    if (!cp.ok) {
        return;
    }
    if (cp.character == '\n') {
        location->line_number += 1;
        location->column_number = 0;
    } else if (cp.character == '\t') {
        location->column_number += 4;
    } else {
        location->column_number += 1;
    }
}

static struct codepoint
next(
    struct arena* arena, struct lexer* lexer, struct location* location,
    bool is_buffer
) {
    struct codepoint character;
    if (lexer->c[0].ok) {
        character      = lexer->c[0];
        lexer->c[0]    = lexer->c[1];
        lexer->c[1].ok = false;
    } else {
        character = utf8_get(lexer->in);
        update_line_number(&lexer->location, character);
        if (!character.ok && !feof(lexer->in)) {
            error(arena, lexer->location, "Invalid UTF-8 character");
        }
    }

    if (location != nullptr) {
        *location = lexer->location;
        for (i32 i = 0; i < 2 && lexer->c[i].ok; ++i) {
            update_line_number(&lexer->location, lexer->c[i]);
        }
    }

    if (!character.ok || !is_buffer) {
        return character;
    }
    struct utf8char encode_result = utf8_encode(character);
    append_buffer(arena, lexer, encode_result);
    return character;
}

static bool
is_whitespace(u8 c) {
    switch (c) {
        case '\t':
        case '\n':
        case '\r':
        case ' ':
            return true;
    }
    return false;
}

static struct codepoint
wgetc(struct arena* arena, struct lexer* lexer, struct location* location) {
    struct codepoint cp;
    do {
        cp = next(arena, lexer, location, false);
    } while (cp.ok && is_whitespace(cp.character));
    return cp;
}

static void
consume(struct lexer* lexer, usize n) {
    for (usize i = 0; i < n; i += 1) {
        while ((lexer->buffer[--lexer->buffer_length] & 0xC0) == 0x80)
            ;
    }
    lexer->buffer[lexer->buffer_length] = 0;
}

static void
push(struct lexer* lexer, u32 character, bool is_buffer) {
    assert(!lexer->c[1].ok);
    lexer->c[1]           = lexer->c[0];
    lexer->c[0].character = character;
    lexer->c[0].ok        = true;
    if (is_buffer) {
        consume(lexer, 1);
    }
}

static void
lex_number(struct arena* arena, struct lexer* lexer, struct token* out) {
    struct codepoint cp = next(arena, lexer, &out->location, true);
    u32 character       = cp.character;
    assert(cp.ok && character <= 0x7F && isdigit(character));

    static char const* chars = "0123456789";

    do {
        character = cp.character;
        if (strchr(chars, character)) {
            continue;
        } else {
            break;
        }
    } while ((cp = next(arena, lexer, nullptr, true)).ok);
    push(lexer, character, true);

    out->integer = strtoumax((char*) lexer->buffer, nullptr, 10);

    out->type = TOKEN_INTEGER;
    lexer_clear_buffer(lexer);
}

static void
lex_name(struct arena* arena, struct lexer* lexer, struct token* out) {
    struct codepoint cp = next(arena, lexer, &out->location, true);
    u32 character       = cp.character;
    assert(cp.ok && character <= 0x7F && isalpha(character));
    while (cp.ok) {
        cp        = next(arena, lexer, &out->location, true);
        character = cp.character;
        if (character > 0x7F || (!isalnum(character) && character != '_')) {
            push(lexer, character, true);
            break;
        }
    }

    out->type = TOKEN_NAME;
    out->name = string_duplicate(arena, (struct string){
        .buffer = lexer->buffer,
        .length = lexer->buffer_length,
    });
    lexer_clear_buffer(lexer);
}

static struct utf8char
lex_rune(struct arena* arena, struct lexer* lexer) {
    struct codepoint cp = next(arena, lexer, nullptr, false);
    u32 character       = cp.character;
    assert(cp.ok);

    switch (character) {
        case '\\': {
            struct location location = lexer->location;
            character = next(arena, lexer, nullptr, false).character;
            switch (character) {
                case '0':
                    return (struct utf8char){
                        .characters = { '\0' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'a':
                    return (struct utf8char){
                        .characters = { '\0' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'b':
                    return (struct utf8char){
                        .characters = { '\b' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'r':
                    return (struct utf8char){
                        .characters = { '\r' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 't':
                    return (struct utf8char){
                        .characters = { '\t' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'n':
                    return (struct utf8char){
                        .characters = { '\n' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'f':
                    return (struct utf8char){
                        .characters = { '\f' },
                        .length     = 1,
                        .ok         = true,
                    };
                case 'v':
                    return (struct utf8char){
                        .characters = { '\v' },
                        .length     = 1,
                        .ok         = true,
                    };
                case '\\':
                    return (struct utf8char){
                        .characters = { '\\' },
                        .length     = 1,
                        .ok         = true,
                    };
                case '\'':
                    return (struct utf8char){
                        .characters = { '\'' },
                        .length     = 1,
                        .ok         = true,
                    };
                case '"':
                    return (struct utf8char){
                        .characters = { '\"' },
                        .length     = 1,
                        .ok         = true,
                    };
                default:
                    error(arena, location, "Invalid escape '\\%c'", character);
            }
            vix_unreachable();
        }
        default:
            return utf8_encode(cp); // TODO
    }
}

static void
lex_string(struct arena* arena, struct lexer* lexer, struct token* out) {
    struct codepoint cp = next(arena, lexer, &out->location, true);
    u32 character       = cp.character;
    u32 delimiter;

    switch (character) {
        case '"':
            delimiter = character;
            cp        = next(arena, lexer, nullptr, false);
            character = cp.character;
            while (character != delimiter) {
                if (!cp.ok) {
                    error(arena, lexer->location, "Unexpected end of file");
                }
                push(lexer, character, false);
                if (delimiter == '"') {
                    struct utf8char utf8 = lex_rune(arena, lexer);
                    append_buffer(arena, lexer, utf8);
                } else {
                    next(arena, lexer, nullptr, true);
                }
                cp        = next(arena, lexer, nullptr, false);
                character = cp.character;
            }
            out->type   = TOKEN_STRING;
            out->string = string_duplicate(
                arena,
                (struct string){
                    .buffer = lexer->buffer + 1,
                    .length = lexer->buffer_length,
                }
            );
            lexer_clear_buffer(lexer);
            return;
        default:
            vix_unreachable();
    }
    vix_unreachable();

    lexer_clear_buffer(lexer);
}

enum lex_token_type
lex(struct arena* arena, struct lexer* lexer, struct token* out) {
    if (lexer->un.type != TOKEN_NONE) {
        *out           = lexer->un;
        lexer->un.type = TOKEN_NONE;
        return out->type;
    }
    struct codepoint cp = wgetc(arena, lexer, &out->location);
    if (!cp.ok) {
        out->type = TOKEN_EOF;
        return out->type;
    }
    u32 character = cp.character;

    if (character <= 0x7F && isdigit(character)) {
        push(lexer, character, false);
        lex_number(arena, lexer, out);
        return TOKEN_INTEGER;
    }

    if (character <= 0x7F && isalpha(character)) {
        push(lexer, character, false);
        lex_name(arena, lexer, out);
        return TOKEN_NAME;
    }

    switch (character) {
        case '"':
            push(lexer, character, false);
            lex_string(arena, lexer, out);
            return TOKEN_STRING;
            break;
        case '.':
            switch ((character = next(arena, lexer, nullptr, false).character)
            ) {
                case '.':
                    switch ((
                        character = next(arena, lexer, nullptr, false).character
                    )) {
                        case '.':
                            out->type = TOKEN_DOT;
                            break;
                        default:
                            error(arena, lexer->location, "Unknown sequence '..'");
                    }
                    break;
                default:
                    push(lexer, character, false);
                    out->type = TOKEN_DOT;
                    break;
            }
            break;
        case '=':
            out->type = TOKEN_ASSIGN;
            break;
        case ',':
            out->type = TOKEN_COMMA;
            break;
        case '{':
            out->type = TOKEN_OPEN_BRACE;
            break;
        case '}':
            out->type = TOKEN_CLOSE_BRACE;
            break;
        case '(':
            out->type = TOKEN_OPEN_PAREN;
            break;
        case ')':
            out->type = TOKEN_CLOSE_PAREN;
            break;
        case '>':
            out->type = TOKEN_GREATER_THAN;
            break;
        case ';':
            out->type = TOKEN_SEMICOLON;
            break;
        default:
            error(arena, lexer->location, "Unexpected character '%d'", character);
    }

    return out->type;
}

void
unlex(struct lexer* lexer, struct token* out) {
    assert(lexer->un.type == TOKEN_NONE);
    lexer->un = *out;
}

void
token_finish(struct token* token) {
    token->type     = TOKEN_NONE;
    token->location = (struct location){};
}
