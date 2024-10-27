#include "parser.h"

#include "allocator.h"
#include "lexer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static u32 id = 0;

static noreturn void
vsynerror(struct arena* arena, struct token* token, va_list ap) {
    enum lex_token_type type = va_arg(ap, enum lex_token_type);

    fprintf(
        stderr, "%s:%d:%d: syntax error: expected ",
        sources[token->location.file], token->location.line_number,
        token->location.column_number
    );

    while (type != TOKEN_EOF) {
        if (type == TOKEN_INTEGER || type == TOKEN_NAME) {
            fprintf(stderr, "%s", token_names[type].buffer);
        } else {
            fprintf(stderr, "'%s'", token_names[type].buffer);
        }

        type = va_arg(ap, enum lex_token_type);
        fprintf(stderr, ", ");
    }

    fprintf(stderr, "found '%s'\n", token_names[token->type].buffer);
    error_line(arena, token->location);
    exit(EXIT_PARSE);
}

static noreturn void
synerror(struct arena* arena, struct token* token, ...) {
    va_list ap;
    va_start(ap, token);
    vsynerror(arena, token, ap);
    va_end(ap);
}

static void
synassert(struct arena* arena, bool condition, struct token* token, ...) {
    if (!condition) {
        va_list ap;
        va_start(ap, token);
        vsynerror(arena, token, ap);
        va_end(ap);
    }
}

static void
expect_token(
    struct arena* arena, struct lexer* lexer, enum lex_token_type token_type,
    struct token* token
) {
    struct token _token = {};
    struct token* out   = token != nullptr ? token : &_token;
    lex(arena, lexer, out);
    synassert(arena, out->type == token_type, out, token_type, TOKEN_EOF);
    if (token == nullptr) {
        token_finish(out);
    }
}

static void
parse_free_properties(
    struct arena* arena, struct lexer* lexer,
    struct ast_free_property* free_property
) {
    free_property->next = arena_allocate(arena, sizeof(struct ast_free_property));
    struct token token  = {};
    struct ast_free_property** next = &free_property->next;
    while (true) {
        switch (lex(arena, lexer, &token)) {
            case TOKEN_NAME:
                (*next)->name = string_duplicate(arena, token.name);
                (*next)->id   = id;
                id += 1;
                (*next)->next = arena_allocate(arena, sizeof(struct ast_free_property));
                next          = &(*next)->next;
                break;
            case TOKEN_GREATER_THAN:
                *next = nullptr;
                return;
            default:
                synerror(arena, &token, TOKEN_GREATER_THAN, TOKEN_NAME, TOKEN_EOF);
        }
    }

    vix_unreachable();
}

static struct ast_object* parse_object(
    struct arena* arena, struct lexer* lexer, struct ast_object* parent
);

static void
parse_object_copies(
    struct arena* arena, struct lexer* lexer,
    struct ast_object_copy* object_copy, struct ast_object* parent
) {
    struct ast_object_copy** next = &object_copy;
    struct token token            = {};
    bool cont                     = true;
    while (cont) {
        switch (lex(arena, lexer, &token)) {
            case TOKEN_DOT: {
                (*next)->next       = arena_allocate(arena, sizeof(struct ast_object_copy));
                next                = &(*next)->next;
                struct token token2 = {};
                expect_token(arena, lexer, TOKEN_NAME, &token2);
                (*next)->name = string_duplicate(arena, token2.name);
                break;
            }
            case TOKEN_OPEN_PAREN: {
                (*next)->free_properties
                    = arena_allocate(arena, sizeof(struct ast_free_property_assign));
                struct ast_free_property_assign** next_free_property_assign
                    = &(*next)->free_properties;
                struct token token2      = {};
                enum lex_token_type type = lex(arena, lexer, &token2);
                if (type == TOKEN_CLOSE_PAREN) {
                    *next_free_property_assign = nullptr;
                    break;
                } else {
                    unlex(lexer, &token2);
                    (*next_free_property_assign)->value
                        = parse_object(arena, lexer, parent);
                    (*next_free_property_assign)->next
                        = arena_allocate(arena, sizeof(struct ast_free_property_assign));
                    next_free_property_assign
                        = &(*next_free_property_assign)->next;
                }
                bool break_out = false;
                while (!break_out) {
                    switch (lex(arena, lexer, &token2)) {
                        case TOKEN_CLOSE_PAREN:
                            *next_free_property_assign = nullptr;
                            break_out                  = true;
                            break;
                        case TOKEN_COMMA:
                            (*next_free_property_assign)->value
                                = parse_object(arena, lexer, parent);
                            (*next_free_property_assign)->next = arena_allocate(
                                arena, sizeof(struct ast_free_property_assign)
                            );
                            next_free_property_assign
                                = &(*next_free_property_assign)->next;
                            break;
                        default:
                            synerror(
                                arena, &token2, TOKEN_CLOSE_PAREN, TOKEN_COMMA,
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
                unlex(lexer, &token);
                cont = false;
                break;
                // synerror(
                //     &token, TOKEN_DOT, TOKEN_OPEN_PAREN, TOKEN_SEMICOLON,
                //     TOKEN_EOF
                // );
            }
        }
    }
}

static struct ast_property* parse_property(
    struct arena* arena, struct lexer* lexer, struct ast_object* parent
);

// object ::= (identifier+ ">")? (("{" (property)* "}") | object_copy |
// primitive)
static struct ast_object*
parse_object(
    struct arena* arena, struct lexer* lexer, struct ast_object* parent
) {
    struct token token        = {};
    struct ast_object* object = arena_allocate(arena, sizeof(struct ast_object));
    object->id                = id;
    id += 1;
    object->parent = parent;

    switch (lex(arena, lexer, &token)) {
        case TOKEN_NAME: {
            // name can mean both identifiers or object copy
            struct token token2   = {};
            enum lex_token_type t = lex(arena, lexer, &token2);
            switch (t) {
                case TOKEN_NAME: { // more than one free property
                    unlex(lexer, &token2);
                    object->free_properties
                        = arena_allocate(arena, sizeof(struct ast_free_property));
                    object->free_properties->name
                        = string_duplicate(arena, token.name);
                    object->free_properties->id = id;
                    id += 1;
                    parse_free_properties(
                        arena, lexer, object->free_properties
                    );
                    break;
                }
                case TOKEN_GREATER_THAN: { // only one free property
                    object->free_properties
                        = arena_allocate(arena, sizeof(struct ast_free_property));
                    object->free_properties->name
                        = string_duplicate(arena, token.name);
                    object->free_properties->id = id;
                    id += 1;
                    break;
                }
                default: { // object copy
                    object->type = OBJECT_TYPE_OBJECT_COPY;
                    unlex(lexer, &token2);
                    struct ast_object_copy* object_copy
                        = arena_allocate(arena, sizeof(struct ast_object_copy));
                    object->object_copy = object_copy;
                    object_copy->name
                        = string_duplicate(arena, token.name);
                    parse_object_copies(arena, lexer, object_copy, object);
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
    switch (lex(arena, lexer, &token)) {
        case TOKEN_NAME:
            object->type = OBJECT_TYPE_OBJECT_COPY;
            struct ast_object_copy* object_copy
                = arena_allocate(arena, sizeof(struct ast_object_copy));
            object->object_copy = object_copy;
            object_copy->name   = string_duplicate(arena, token.name);
            parse_object_copies(arena, lexer, object_copy, object);
            break;
            break;
        case TOKEN_OPEN_BRACE: {
            object->type        = OBJECT_TYPE_PROPERTIES;
            struct token token2 = {};
            object->properties  = arena_allocate(arena, sizeof(struct ast_property));
            struct ast_property** next_property = &object->properties;
            bool break_out                      = false;
            while (!break_out) {
                switch (lex(arena, lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        *next_property = nullptr;
                        break_out      = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        *next_property = parse_property(arena, lexer, object);
                        (*next_property)->next
                            = arena_allocate(arena, sizeof(struct ast_property));
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
            object->string = string_duplicate(arena, token.string);
            break;
        default:
            unlex(lexer, &token);
            break;
    }

    return object;
}

// property ::= identifier "=" object ";"
static struct ast_property*
parse_property(
    struct arena* arena, struct lexer* lexer, struct ast_object* parent
) {
    struct token token       = {};
    enum lex_token_type type = lex(arena, lexer, &token);
    if (type != TOKEN_NAME) {
        unlex(lexer, &token);
        return nullptr;
    }
    struct ast_property* property = arena_allocate(arena, sizeof(struct ast_property));
    property->name                = string_duplicate(arena, token.name);

    expect_token(arena, lexer, TOKEN_ASSIGN, nullptr);

    property->object = parse_object(arena, lexer, parent);

    expect_token(arena, lexer, TOKEN_SEMICOLON, nullptr);

    return property;
}

static struct ast_object*
parse_root(struct arena* arena, struct lexer* lexer) {
    struct ast_object* root = arena_allocate(arena, sizeof(struct ast_object));
    root->id                = id;
    id += 1;
    root->type       = OBJECT_TYPE_PROPERTIES;
    root->properties = arena_allocate(arena, sizeof(struct ast_property));
    struct ast_property** next_property = &root->properties;

    while (true) {
        *next_property = parse_property(arena, lexer, root);
        if (*next_property == nullptr) {
            break;
        }
        (*next_property)->next = arena_allocate(arena, sizeof(struct ast_property));
        next_property          = &(*next_property)->next;
    }
    *next_property = nullptr;
    return root;
}

struct ast_object*
parse(struct arena* arena, struct lexer* lexer) {
    struct ast_object* root = parse_root(arena, lexer);
    expect_token(arena, lexer, TOKEN_EOF, nullptr);
    return root;
}

static void
print_indent(usize indent) {
    for (usize i = 0; i < indent; ++i) {
        fprintf(stdout, " ");
    }
}

static void
print_free_property(struct ast_free_property* free_property, usize indent) {
    if (free_property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Free Property: %s\n", free_property->name.buffer);
    print_free_property(free_property->next, indent);
}

static void
print_property(struct ast_property* property, usize indent) {
    if (property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Property: %s\n", property->name.buffer);
    print_object(property->object, indent + 2);
    print_property(property->next, indent);
}

static void
print_free_property_assign(
    struct ast_free_property_assign* free_property_assign, usize indent
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
print_object_copy(struct ast_object_copy* object_copy, usize indent) {
    if (object_copy == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object Copy: %s\n", object_copy->name.buffer);
    print_free_property_assign(object_copy->free_properties, indent + 2);
    print_object_copy(object_copy->next, indent);
}

void
print_object(struct ast_object* object, usize indent) {
    if (object == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object %d", object->id);
    if (object->parent != nullptr) {
        fprintf(stdout, ", parent %d", object->parent->id);
    }
    fprintf(stdout, ":\n");
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
            fprintf(stdout, "  value: %ld\n", object->integer);
            break;
        case OBJECT_TYPE_STRING:
            print_indent(indent);
            fprintf(stdout, "  value: %s\n", object->string.buffer);
            break;
        case OBJECT_TYPE_NONE:
            break;
    }
}
