#include "parser.h"

#include "util.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static i32 id = 0;

static noreturn void
vsynerror(struct token token[static 1], va_list ap) {
    enum lex_token_type type = va_arg(ap, enum lex_token_type);

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

        type = va_arg(ap, enum lex_token_type);
        fprintf(stderr, ", ");
    }

    fprintf(stderr, "found '%s'\n", token_names[token->type]);
    error_line(token->location);
    exit(EXIT_PARSE);
}

static noreturn void
synerror(struct token token[static 1], ...) {
    va_list ap;
    va_start(ap, token);
    vsynerror(token, ap);
    va_end(ap);
}

static void
synassert(bool condition, struct token token[static 1], ...) {
    if (!condition) {
        va_list ap;
        va_start(ap, token);
        vsynerror(token, ap);
        va_end(ap);
    }
}

static void
expect_token(
    struct lexer lexer[static 1], enum lex_token_type token_type,
    struct token* token
) {
    struct token _token = {};
    struct token* out   = token != nullptr ? token : &_token;
    lex(lexer, out);
    synassert(out->type == token_type, out, token_type, TOKEN_EOF);
    if (token == nullptr) {
        token_finish(out);
    }
}

static void
parse_free_properties(
    struct lexer lexer[static 1],
    struct ast_free_property free_property[static 1]
) {
    free_property->next = calloc(1, sizeof(struct ast_free_property));
    struct token token  = {};
    struct ast_free_property** next = &free_property->next;
    while (true) {
        switch (lex(lexer, &token)) {
            case TOKEN_NAME:
                (*next)->name = token.name;
                (*next)->id   = id;
                id += 1;
                (*next)->next = calloc(1, sizeof(struct ast_free_property));
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

static struct ast_object* parse_object(
    struct lexer lexer[static 1], struct ast_object* parent
);

static void
parse_object_copies(
    struct lexer lexer[static 1], struct ast_object_copy object_copy[static 1],
    struct ast_object* parent
) {
    struct ast_object_copy** next = &object_copy;
    struct token token            = {};
    bool cont                     = true;
    while (cont) {
        switch (lex(lexer, &token)) {
            case TOKEN_DOT: {
                (*next)->next       = calloc(1, sizeof(struct ast_object_copy));
                next                = &(*next)->next;
                struct token token2 = {};
                expect_token(lexer, TOKEN_NAME, &token2);
                (*next)->name = token2.name;
                break;
            }
            case TOKEN_OPEN_PAREN: {
                (*next)->free_properties
                    = calloc(1, sizeof(struct ast_free_property_assign));
                struct ast_free_property_assign** next_free_property_assign
                    = &(*next)->free_properties;
                struct token token2      = {};
                enum lex_token_type type = lex(lexer, &token2);
                if (type == TOKEN_CLOSE_PAREN) {
                    free(*next_free_property_assign);
                    *next_free_property_assign = nullptr;
                    break;
                } else {
                    unlex(lexer, &token2);
                    (*next_free_property_assign)->value
                        = parse_object(lexer, parent);
                    (*next_free_property_assign)->next
                        = calloc(1, sizeof(struct ast_free_property_assign));
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
                                = parse_object(lexer, parent);
                            (*next_free_property_assign)->next = calloc(
                                1, sizeof(struct ast_free_property_assign)
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
    struct lexer lexer[static 1], struct ast_object* parent
);

// object ::= (identifier+ ">")? (("{" (property)* "}") | object_copy |
// primitive)
static struct ast_object*
parse_object(struct lexer lexer[static 1], struct ast_object* parent) {
    struct token token        = {};
    struct ast_object* object = calloc(1, sizeof(struct ast_object));
    object->id                = id;
    id += 1;
    object->parent = parent;

    switch (lex(lexer, &token)) {
        case TOKEN_NAME: {
            // name can mean both identifiers or object copy
            struct token token2   = {};
            enum lex_token_type t = lex(lexer, &token2);
            switch (t) {
                case TOKEN_NAME: { // more than one free property
                    unlex(lexer, &token2);
                    object->free_properties
                        = calloc(1, sizeof(struct ast_free_property));
                    object->free_properties->name = token.name;
                    object->free_properties->id   = id;
                    id += 1;
                    parse_free_properties(lexer, object->free_properties);
                    break;
                }
                case TOKEN_GREATER_THAN: { // only one free property
                    object->free_properties
                        = calloc(1, sizeof(struct ast_free_property));
                    object->free_properties->name = token.name;
                    object->free_properties->id   = id;
                    id += 1;
                    break;
                }
                default: { // object copy
                    object->type = OBJECT_TYPE_OBJECT_COPY;
                    unlex(lexer, &token2);
                    struct ast_object_copy* object_copy
                        = calloc(1, sizeof(struct ast_object_copy));
                    object->object_copy = object_copy;
                    object_copy->name   = token.name;
                    parse_object_copies(lexer, object_copy, object);
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
            struct ast_object_copy* object_copy
                = calloc(1, sizeof(struct ast_object_copy));
            object->object_copy = object_copy;
            object_copy->name   = token.name;
            parse_object_copies(lexer, object_copy, object);
            break;
            break;
        case TOKEN_OPEN_BRACE: {
            object->type        = OBJECT_TYPE_PROPERTIES;
            struct token token2 = {};
            object->properties  = calloc(1, sizeof(struct ast_property));
            struct ast_property** next_property = &object->properties;
            bool break_out                      = false;
            while (!break_out) {
                switch (lex(lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        free(*next_property);
                        *next_property = nullptr;
                        break_out      = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        *next_property = parse_property(lexer, object);
                        (*next_property)->next
                            = calloc(1, sizeof(struct ast_property));
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
static struct ast_property*
parse_property(struct lexer lexer[static 1], struct ast_object* parent) {
    struct token token       = {};
    enum lex_token_type type = lex(lexer, &token);
    if (type != TOKEN_NAME) {
        unlex(lexer, &token);
        return nullptr;
    }
    struct ast_property* property = calloc(1, sizeof(struct ast_property));
    property->name                = token.name;

    expect_token(lexer, TOKEN_ASSIGN, nullptr);

    property->object = parse_object(lexer, parent);

    expect_token(lexer, TOKEN_SEMICOLON, nullptr);

    return property;
}

static struct ast_object*
parse_root(struct lexer lexer[static 1]) {
    struct ast_object* root = calloc(1, sizeof(struct ast_object));
    root->id                = id;
    id += 1;
    root->type       = OBJECT_TYPE_PROPERTIES;
    root->properties = calloc(1, sizeof(struct ast_property));
    struct ast_property** next_property = &root->properties;

    while (true) {
        *next_property = parse_property(lexer, root);
        if (*next_property == nullptr) {
            break;
        }
        (*next_property)->next = calloc(1, sizeof(struct ast_property));
        next_property          = &(*next_property)->next;
    }
    free(*next_property);
    *next_property = nullptr;
    return root;
}

struct ast_object*
parse(struct lexer lexer[static 1]) {
    struct ast_object* root = parse_root(lexer);
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
print_free_property(struct ast_free_property* free_property, i32 indent) {
    if (free_property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Free Property: %s\n", free_property->name);
    print_free_property(free_property->next, indent);
}

static void
print_property(struct ast_property* property, i32 indent) {
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
    struct ast_free_property_assign* free_property_assign, i32 indent
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
print_object_copy(struct ast_object_copy* object_copy, i32 indent) {
    if (object_copy == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object Copy: %s\n", object_copy->name);
    print_free_property_assign(object_copy->free_properties, indent + 2);
    print_object_copy(object_copy->next, indent);
}

void
print_object(struct ast_object* object, i32 indent) {
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
