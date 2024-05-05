#include "parser.h"

#include "util.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static noreturn void
vsynerror(struct token_t token[static 1], va_list ap) {
    enum lex_token_type_e type = va_arg(ap, enum lex_token_type_e);

    fprintf(
        stderr, "%s:%d:%d: syntax error: expected ",
        sources[token->location.file], token->location.line_number,
        token->location.column_number
    );

    while (type != TOKEN_EOF) {
        if (type == TOKEN_INTEGER || type == TOKEN_NAME) {
            fprintf(stderr, "%s", token_names[type]);
        } else {
            fprintf(stderr, "'%s'", token_names[type]);
        }

        type = va_arg(ap, enum lex_token_type_e);
        fprintf(stderr, ", ");
    }

    fprintf(stderr, "found '%s'\n", token_names[token->type]);
    error_line(token->location);
    exit(EXIT_PARSE);
}

static noreturn void
synerror(struct token_t token[static 1], ...) {
    va_list ap;
    va_start(ap, token);
    vsynerror(token, ap);
    va_end(ap);
}

static void
synassert(bool condition, struct token_t token[static 1], ...) {
    if (!condition) {
        va_list ap;
        va_start(ap, token);
        vsynerror(token, ap);
        va_end(ap);
    }
}

static void
expect_token(
    struct lexer_t lexer[static 1], enum lex_token_type_e token_type,
    struct token_t* token
) {
    struct token_t _token = {};
    struct token_t* out   = token != nullptr ? token : &_token;
    lex(lexer, out);
    synassert(out->type == token_type, out, token_type, TOKEN_EOF);
    if (token == nullptr) {
        token_finish(out);
    }
}

static void
parse_free_properties(
    struct lexer_t lexer[static 1],
    struct ast_free_property_t free_property[static 1]
) {
    free_property->next  = malloc(sizeof(struct ast_free_property_t));
    struct token_t token = {};
    struct ast_free_property_t** next = &free_property->next;
    while (true) {
        switch (lex(lexer, &token)) {
            case TOKEN_NAME:
                (*next)->name = token.name;
                (*next)->next = malloc(sizeof(struct ast_free_property_t));
                next          = &(*next)->next;
                break;
            case TOKEN_GREATER_THAN:
                free(*next);
                *next = nullptr;
                return;
            default:
                synerror(&token, TOKEN_GREATER_THAN, TOKEN_NAME, TOKEN_EOF);
        }
    }

    vix_unreachable();
}

static struct ast_object_t* parse_object(struct lexer_t lexer[static 1]);

static void
parse_object_copies(
    struct lexer_t lexer[static 1],
    struct ast_object_copy_t object_copy[static 1]
) {
    struct ast_object_copy_t** next = &object_copy;
    struct token_t token            = {};
    while (true) {
        switch (lex(lexer, &token)) {
            case TOKEN_DOT: {
                (*next)->next = malloc(sizeof(struct ast_object_copy_t));
                next          = &(*next)->next;
                struct token_t token2 = {};
                expect_token(lexer, TOKEN_NAME, &token2);
                (*next)->name = token2.name;
                break;
            }
            case TOKEN_OPEN_PAREN: {
                (*next)->free_properties
                    = malloc(sizeof(struct ast_free_property_assign_t));
                struct ast_free_property_assign_t** next_free_property_assign
                    = &(*next)->free_properties;
                struct token_t token2      = {};
                enum lex_token_type_e type = lex(lexer, &token2);
                if (type == TOKEN_CLOSE_PAREN) {
                    free(*next_free_property_assign);
                    *next_free_property_assign = nullptr;
                    break;
                } else {
                    unlex(lexer, &token2);
                    (*next_free_property_assign)->value = parse_object(lexer);
                    (*next_free_property_assign)->next
                        = malloc(sizeof(struct ast_free_property_assign_t));
                    next_free_property_assign
                        = &(*next_free_property_assign)->next;
                }
                bool break_out = false;
                while (!break_out) {
                    switch (lex(lexer, &token2)) {
                        case TOKEN_CLOSE_PAREN:
                            free(*next_free_property_assign);
                            *next_free_property_assign = nullptr;
                            break_out                  = true;
                            break;
                        case TOKEN_COMMA:
                            (*next_free_property_assign)->value
                                = parse_object(lexer);
                            (*next_free_property_assign)->next = malloc(
                                sizeof(struct ast_free_property_assign_t)
                            );
                            next_free_property_assign
                                = &(*next_free_property_assign)->next;
                            break;
                        default:
                            synerror(
                                &token2, TOKEN_CLOSE_PAREN, TOKEN_COMMA,
                                TOKEN_EOF
                            );
                    }
                }
                break;
            }
            case TOKEN_SEMICOLON: {
                unlex(lexer, &token);
                return;
            }
            default: {
                synerror(
                    &token, TOKEN_DOT, TOKEN_OPEN_PAREN, TOKEN_SEMICOLON,
                    TOKEN_EOF
                );
            }
        }
    }
}

static struct ast_property_t* parse_property(struct lexer_t lexer[static 1]);

// object ::= (identifier+ ">")? (("{" (property)* "}") | object_copy |
// primitive)
static struct ast_object_t*
parse_object(struct lexer_t lexer[static 1]) {
    struct token_t token        = {};
    struct ast_object_t* object = malloc(sizeof(struct ast_object_t));

    switch (lex(lexer, &token)) {
        case TOKEN_NAME: {
            // name can mean both identifiers or object copy
            struct token_t token2 = {};
            switch (lex(lexer, &token2)) {
                case TOKEN_NAME: { // more than one free property
                    unlex(lexer, &token2);
                    object->free_properties
                        = malloc(sizeof(struct ast_free_property_t));
                    object->free_properties->name = token.name;
                    parse_free_properties(lexer, object->free_properties);
                    break;
                }
                case TOKEN_GREATER_THAN: { // only one free property
                    object->free_properties
                        = malloc(sizeof(struct ast_free_property_t));
                    object->free_properties->name = token.name;
                    break;
                }
                default: { // object copy
                    object->type = OBJECT_TYPE_OBJECT_COPY;
                    unlex(lexer, &token2);
                    struct ast_object_copy_t* object_copy
                        = malloc(sizeof(struct ast_object_copy_t));
                    object->object_copy = object_copy;
                    object_copy->name   = token.name;
                    parse_object_copies(lexer, object_copy);
                    break;
                }
            }
            break;
        }
        default:
            unlex(lexer, &token);
            break;
    }
    if (object->type == OBJECT_TYPE_OBJECT_COPY) {
        return object;
    }
    switch (lex(lexer, &token)) {
        case TOKEN_NAME:
            object->type = OBJECT_TYPE_OBJECT_COPY;
            struct ast_object_copy_t* object_copy
                = malloc(sizeof(struct ast_object_copy_t));
            object->object_copy = object_copy;
            object_copy->name   = token.name;
            parse_object_copies(lexer, object_copy);
            break;
            break;
        case TOKEN_OPEN_BRACE: {
            object->type          = OBJECT_TYPE_PROPERTIES;
            struct token_t token2 = {};
            object->properties    = malloc(sizeof(struct ast_property_t));
            struct ast_property_t** next_property = &object->properties;
            bool break_out                        = false;
            while (!break_out) {
                switch (lex(lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        free(*next_property);
                        *next_property = nullptr;
                        break_out      = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        *next_property = parse_property(lexer);
                        (*next_property)->next
                            = malloc(sizeof(struct ast_property_t));
                        next_property = &(*next_property)->next;
                        break;
                }
            }
            break;
        }
        case TOKEN_INTEGER:
            object->type    = OBJECT_TYPE_INTEGER;
            object->integer = token.integer;
            break;
        case TOKEN_STRING:
            object->type   = OBJECT_TYPE_STRING;
            object->string = token.string.value;
            break;
        default:
            unlex(lexer, &token);
            break;
    }

    return object;
}

// property ::= identifier "=" object ";"
static struct ast_property_t*
parse_property(struct lexer_t lexer[static 1]) {
    struct token_t token       = {};
    enum lex_token_type_e type = lex(lexer, &token);
    if (type != TOKEN_NAME) {
        unlex(lexer, &token);
        return nullptr;
    }
    struct ast_property_t* property = malloc(sizeof(struct ast_property_t));
    property->name                  = token.name;

    expect_token(lexer, TOKEN_ASSIGN, nullptr);

    property->object = parse_object(lexer);

    expect_token(lexer, TOKEN_SEMICOLON, nullptr);

    return property;
}

static struct ast_object_t*
parse_root(struct lexer_t lexer[static 1]) {
    struct ast_object_t* object = malloc(sizeof(struct ast_object_t));
    object->type                = OBJECT_TYPE_PROPERTIES;
    object->properties          = malloc(sizeof(struct ast_property_t));
    struct ast_property_t** next_property = &object->properties;

    while (true) {
        *next_property = parse_property(lexer);
        if (*next_property == nullptr) {
            break;
        }
        (*next_property)->next = malloc(sizeof(struct ast_property_t));
        next_property          = &(*next_property)->next;
    }
    free(*next_property);
    *next_property = nullptr;
    return object;
}

struct ast_object_t*
parse(struct lexer_t lexer[static 1]) {
    struct ast_object_t* root = parse_root(lexer);
    expect_token(lexer, TOKEN_EOF, nullptr);
    return root;
}

static void
print_indent(i32 indent) {
    for (i32 i = 0; i < indent; ++i) {
        fprintf(stdout, " ");
    }
}

static void
print_free_property(struct ast_free_property_t* free_property, i32 indent) {
    if (free_property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Free Property: %s\n", free_property->name);
    print_free_property(free_property->next, indent);
}

static void
print_property(struct ast_property_t* property, i32 indent) {
    if (property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Property: %s\n", property->name);
    print_object(property->object, indent + 2);
    print_property(property->next, indent);
}

static void
print_free_property_assign(
    struct ast_free_property_assign_t* free_property_assign, i32 indent
) {
    if (free_property_assign == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Free Property Assign:\n");
    print_object(free_property_assign->value, indent + 2);
    print_free_property_assign(free_property_assign->next, indent);
}

static void
print_object_copy(struct ast_object_copy_t* object_copy, i32 indent) {
    if (object_copy == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object Copy: %s\n", object_copy->name);
    print_free_property_assign(object_copy->free_properties, indent + 2);
    print_object_copy(object_copy->next, indent);
}

void
print_object(struct ast_object_t* object, i32 indent) {
    if (object == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object:\n");
    print_free_property(object->free_properties, indent + 2);
    switch (object->type) {
        case OBJECT_TYPE_OBJECT_COPY:
            print_object_copy(object->object_copy, indent + 2);
            break;
        case OBJECT_TYPE_PROPERTIES:
            print_property(object->properties, indent + 2);
            break;
        case OBJECT_TYPE_INTEGER:
            print_indent(indent);
            fprintf(stdout, "  value: %d\n", object->integer);
            break;
        case OBJECT_TYPE_STRING:
            print_indent(indent);
            fprintf(stdout, "  value: %s\n", object->string);
            break;
        case OBJECT_TYPE_NONE:
            break;
    }
}
