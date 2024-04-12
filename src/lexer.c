#include "lexer.h"

#include "array.h"
#include "string.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define WHITESPACE   \
    ' ' : case '\t': \
    case '\r':       \
    case '\n'

#define DIGIT       \
    '0' : case '1': \
    case '2':       \
    case '3':       \
    case '4':       \
    case '5':       \
    case '6':       \
    case '7':       \
    case '8':       \
    case '9'

#define ALPHA       \
    'a' : case 'b': \
    case 'c':       \
    case 'd':       \
    case 'e':       \
    case 'f':       \
    case 'g':       \
    case 'h':       \
    case 'i':       \
    case 'j':       \
    case 'k':       \
    case 'l':       \
    case 'm':       \
    case 'n':       \
    case 'o':       \
    case 'p':       \
    case 'q':       \
    case 'r':       \
    case 's':       \
    case 't':       \
    case 'u':       \
    case 'v':       \
    case 'w':       \
    case 'x':       \
    case 'y':       \
    case 'z':       \
    case 'A':       \
    case 'B':       \
    case 'C':       \
    case 'D':       \
    case 'E':       \
    case 'F':       \
    case 'G':       \
    case 'H':       \
    case 'I':       \
    case 'J':       \
    case 'K':       \
    case 'L':       \
    case 'M':       \
    case 'N':       \
    case 'O':       \
    case 'P':       \
    case 'Q':       \
    case 'R':       \
    case 'S':       \
    case 'T':       \
    case 'U':       \
    case 'V':       \
    case 'W':       \
    case 'X':       \
    case 'Y':       \
    case 'Z'

#define SYMBOL_CHAR \
    ALPHA:          \
    case DIGIT:     \
    case '_'

enum tokenize_state_t {
    TOKENIZE_STATE_START,
    TOKENIZE_STATE_STRING,
    TOKENIZE_STATE_STRING_ESCAPE,
    TOKENIZE_STATE_CHAR,
    TOKENIZE_STATE_CHAR_END,
    TOKENIZE_STATE_IDENTIFIER,
    TOKENIZE_STATE_SAW_DOT,
    TOKENIZE_STATE_SAW_DOT_DOT,
    TOKENIZE_STATE_SAW_DIGIT,
    TOKENIZE_STATE_ERROR,
};

struct tokenize_t {
    struct str_t source;
    struct tokenized_t* const out;
    token_ptr_array_t* tokens;
    struct allocator_t* allocator;

    size_t position;
    enum tokenize_state_t state;
    size_t line;
    size_t column;
    struct token_t* current_token;
};

static void set_token_type(
    struct token_t token[static const 1], enum token_type_t const type
) {
    token->type = type;

    if (type == TOKEN_STRING_LITERAL) {
        token->data.string_literal.string = str_new_empty();
    } else if (type == TOKEN_IDENTIFIER) {
        token->data.identifier.identifier = str_new_empty();
    } else if (type == TOKEN_INT) {
        token->data.integer.integer = str_new_empty();
    }
}

static void begin_token(
    struct tokenize_t t[static const 1], enum token_type_t const type
) {
    assert(!t->current_token);
    struct token_t* token = (struct token_t*) t->allocator->allocate(
        sizeof(struct token_t), t->allocator->context
    );
    token->start_line     = t->line;
    token->start_column   = t->column;
    token->start_position = t->position,
    set_token_type(token, type);
    array_push(*t->tokens, token);

    t->current_token = token;
}

static void end_token(struct tokenize_t t[static const 1]) {
    assert(t->current_token);
    t->current_token->end_position = t->position + 1;

    // if (t->current_token

    t->current_token = NULL;
}

static void tokenize_error(
    struct tokenize_t t[static const 1],
    struct str_t const str
) {
    t->state = TOKENIZE_STATE_ERROR;

    if (t->current_token) {
        t->out->error_line   = t->current_token->start_line;
        t->out->error_column = t->current_token->start_column;
    } else {
        t->out->error_line   = t->line;
        t->out->error_column = t->column;
    }

    t->out->error = str;
}

static void handle_string_escape(
    struct tokenize_t t[static const 1],
    struct str_buffer_t str_buffer[static const 1], char const c
) {
    if (t->current_token->type == TOKEN_CHAR_LITERAL) {
        t->current_token->data.char_literal.c = c;
        t->state                              = TOKENIZE_STATE_CHAR_END;
    } else if (t->current_token->type == TOKEN_STRING_LITERAL || t->current_token->type == TOKEN_IDENTIFIER) {
        str_buffer_append_char(str_buffer, c);
        t->state = TOKENIZE_STATE_STRING;
    } else {
        vix_unreachable();
    }
}

void tokenize(
    struct allocator_t allocator[static const 1], struct str_t const source,
    token_ptr_array_t tokens[static const 1], struct tokenized_t out[static 1]
) {
    struct tokenize_t t = {
        .out       = out,
        .tokens    = tokens,
        .source    = source,
        .allocator = allocator,
    };
    out->tokens = t.tokens;

    out->line_offsets = array(size_t, t.allocator);

    struct str_buffer_t buffer = str_buffer_new(allocator, 0);

    array_push(out->line_offsets, 0);
    for (t.position = 0; t.position < t.source.length; t.position++) {
        char const c = str_at(t.source, t.position);
        switch (t.state) {
        case TOKENIZE_STATE_ERROR:
            break;
        case TOKENIZE_STATE_START:
            switch (c) {
            case WHITESPACE:
                break;
            case ';':
                begin_token(&t, TOKEN_SEMICOLON);
                end_token(&t);
                break;
            case '{':
                begin_token(&t, TOKEN_OPEN_BRACE);
                end_token(&t);
                break;
            case '}':
                begin_token(&t, TOKEN_CLOSE_BRACE);
                end_token(&t);
                break;
            case '(':
                begin_token(&t, TOKEN_OPEN_PAREN);
                end_token(&t);
                break;
            case ')':
                begin_token(&t, TOKEN_CLOSE_PAREN);
                end_token(&t);
                break;
            case '=':
                begin_token(&t, TOKEN_ASSIGN);
                end_token(&t);
                break;
            case ',':
                begin_token(&t, TOKEN_COMMA);
                end_token(&t);
                break;
            case '.':
                begin_token(&t, TOKEN_DOT);
                t.state = TOKENIZE_STATE_SAW_DOT;
                break;
            case '>':
                begin_token(&t, TOKEN_GREATER_THAN);
                end_token(&t);
                break;
            case '"':
                begin_token(&t, TOKEN_STRING_LITERAL);
                t.state = TOKENIZE_STATE_STRING;
                break;
            case '\'':
                begin_token(&t, TOKEN_CHAR_LITERAL);
                t.state = TOKENIZE_STATE_CHAR;
                break;
            case ALPHA:
                begin_token(&t, TOKEN_IDENTIFIER);
                str_buffer_append_char(&buffer, c);
                t.state = TOKENIZE_STATE_IDENTIFIER;
                break;
            case DIGIT:
                begin_token(&t, TOKEN_INT);
                str_buffer_append_char(&buffer, c);
                t.state = TOKENIZE_STATE_SAW_DIGIT;
                break;
            default:
                str_buffer_printf(&buffer, str_new("invalid character: '%c'"), c);
                tokenize_error(&t, str_buffer_str(&buffer));
                break;
            }
            break;
        case TOKENIZE_STATE_CHAR:
            switch (c) {
            case '\'':
                str_buffer_printf(&buffer, str_new("expected character"));
                tokenize_error(&t, str_buffer_str(&buffer));
                break;
            case '\\':
                t.state = TOKENIZE_STATE_STRING_ESCAPE;
                break;
            default:
                t.current_token->data.char_literal.c = c;
                t.state                              = TOKENIZE_STATE_CHAR_END;
                break;
            }
            break;
        case TOKENIZE_STATE_CHAR_END:
            switch (c) {
            case '\'':
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            default:
                str_buffer_printf(&buffer, str_new("invalid character: '%c'"), c);
                tokenize_error(&t, str_buffer_str(&buffer));
                break;
            }
            break;
        case TOKENIZE_STATE_STRING:
            switch (c) {
            case '"':
                t.current_token->data.string_literal.string =
                    str_buffer_str(&buffer);
                str_buffer_reset(&buffer);
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            default:
                str_buffer_append_char(&buffer, c);
                break;
            }
            break;
        case TOKENIZE_STATE_STRING_ESCAPE:
            switch (c) {
            case 'n':
                handle_string_escape(&t, &buffer, '\n');
                break;
            case 'r':
                handle_string_escape(&t, &buffer, '\r');
                break;
            case '\\':
                handle_string_escape(&t, &buffer, '\\');
                break;
            case 't':
                handle_string_escape(&t, &buffer, '\t');
                break;
            case '\'':
                handle_string_escape(&t, &buffer, '\'');
                break;
            case '"':
                handle_string_escape(&t, &buffer, '"');
                break;
            default:
                str_buffer_printf(&buffer, str_new("invalid character: '%c'"), c);
                tokenize_error(&t, str_buffer_str(&buffer));
                break;
            }
            break;
        case TOKENIZE_STATE_IDENTIFIER:
            switch (c) {
            case SYMBOL_CHAR:
                str_buffer_append_char(&buffer, c);
                break;
            default:
                t.position -= 1;
                t.column -= 1;
                t.current_token->data.identifier.identifier =
                    str_buffer_str(&buffer);
                str_buffer_reset(&buffer);
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            }
            break;
        case TOKENIZE_STATE_SAW_DOT:
            switch (c) {
            case '.':
                set_token_type(t.current_token, TOKEN_DOT_DOT);
                t.state = TOKENIZE_STATE_SAW_DOT_DOT;
                break;
            default:
                t.position -= 1;
                t.column -= 1;
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            }
            break;
        case TOKENIZE_STATE_SAW_DOT_DOT:
            switch (c) {
            case '.':
                set_token_type(t.current_token, TOKEN_DOT_DOT_DOT);
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            default:
                str_buffer_printf(&buffer, str_new("invalid character: '%c'"), c);
                tokenize_error(&t, str_buffer_str(&buffer));
                break;
            }
            break;
        case TOKENIZE_STATE_SAW_DIGIT:
            switch (c) {
            case DIGIT:
                str_buffer_append_char(&buffer, c);
                break;
            default:
                t.position -= 1;
                t.column -= 1;
                t.current_token->data.integer.integer = str_buffer_str(&buffer);
                str_buffer_reset(&buffer);
                end_token(&t);
                t.state = TOKENIZE_STATE_START;
                break;
            }
            break;
        }
        if (c == '\n') {
            array_push(out->line_offsets, t.position + 1);
            t.line += 1;
            t.column = 0;
        } else {
            t.column += 1;
        }
    }
    // End of file
    switch (t.state) {
    case TOKENIZE_STATE_START:
    case TOKENIZE_STATE_ERROR:
        break;
    case TOKENIZE_STATE_CHAR:
    case TOKENIZE_STATE_CHAR_END:
        str_buffer_printf(&buffer, str_new("unterminated character literal"));
        tokenize_error(&t, str_buffer_str(&buffer));
        break;
    case TOKENIZE_STATE_STRING_ESCAPE:
        if (t.current_token->type == TOKEN_STRING_LITERAL) {
            str_buffer_printf(&buffer, str_new("unterminated string"));
            tokenize_error(&t, str_buffer_str(&buffer));
        } else if (t.current_token->type == TOKEN_CHAR_LITERAL) {
            str_buffer_printf(&buffer, str_new("unterminated character literal"));
            tokenize_error(&t, str_buffer_str(&buffer));
        } else {
            vix_unreachable();
        }
        break;
    case TOKENIZE_STATE_STRING:
        str_buffer_printf(&buffer, str_new("unterminated string"));
        tokenize_error(&t, str_buffer_str(&buffer));
        break;
    case TOKENIZE_STATE_IDENTIFIER:
    case TOKENIZE_STATE_SAW_DIGIT:
    case TOKENIZE_STATE_SAW_DOT:
    case TOKENIZE_STATE_SAW_DOT_DOT:
        end_token(&t);
        break;
    }

    if (t.state != TOKENIZE_STATE_ERROR) {
        if (array_header(*t.tokens)->length > 0) {
            struct token_t* const last_token = array_last(*t.tokens);
            t.line                           = last_token->start_line;
            t.column                         = last_token->start_column;
            t.position                       = last_token->start_position;
        } else {
            t.position = 0;
        }
        begin_token(&t, TOKEN_EOF);
        end_token(&t);
        assert(!t.current_token);
    }
}

char const* token_name(enum token_type_t const type) {
    switch (type) {
    case TOKEN_INT:
        return "Integer";
    case TOKEN_STRING_LITERAL:
        return "StringLiteral";
    case TOKEN_IDENTIFIER:
        return "Identifier";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_ASSIGN:
        return "=";
    case TOKEN_SEMICOLON:
        return ";";
    case TOKEN_DOT:
        return ".";
    case TOKEN_DOT_DOT:
        return "..";
    case TOKEN_DOT_DOT_DOT:
        return "...";
    case TOKEN_COMMA:
        return ",";
    case TOKEN_OPEN_BRACE:
        return "{";
    case TOKEN_CLOSE_BRACE:
        return "}";
    case TOKEN_OPEN_PAREN:
        return "(";
    case TOKEN_CLOSE_PAREN:
        return ")";
    case TOKEN_GREATER_THAN:
        return ">";
    case TOKEN_CHAR_LITERAL:
        return "CharLiteral";
    }
    return "(invalid token)";
}

void print_tokens(struct str_t source, struct token_t** tokens) {
    for (size_t i = 0; i < array_header(tokens)->length; ++i) {
        struct token_t const* const token = tokens[i];
        printf("%s ", token_name(token->type));
        if (token->start_position != __SIZE_MAX__) {
            fwrite(
                source.data + token->start_position, sizeof(char),
                token->end_position - token->start_position, stdout
            );
        }
        printf("\n");
    }
}
