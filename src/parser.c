#include "allocator.h"
#include "ast.h"
#include "lexer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static u32 id = 0;

static struct ast_object*
create_object(struct arena* arena) {
    struct ast_object* new_object
        = arena_allocate(arena, sizeof(struct ast_object));
    new_object->alias = AST_OBJECT_ALIAS_NONE;
    new_object->id    = id;
    id += 1;
    return new_object;
}

static struct ast_property*
create_property(struct arena* arena, enum ast_property_type type) {
    struct ast_property* new_property
        = arena_allocate(arena, sizeof(struct ast_property));
    new_property->type = type;
    new_property->id   = id;
    id += 1;
    return new_property;
}

static noreturn void
vsynerror(struct arena* arena, struct token* token, va_list ap) {
    enum lex_token_type type = va_arg(ap, enum lex_token_type);

    fprintf(
        stderr, STR_FMT ":%d:%d: syntax error: expected ",
        STR_ARG(sources[token->location.file]), token->location.line_number,
        token->location.column_number
    );

    while (type != TOKEN_EOF) {
        if (type == TOKEN_INTEGER || type == TOKEN_NAME) {
            fprintf(stderr, STR_FMT, STR_ARG(token_names[type]));
        } else {
            fprintf(stderr, "'" STR_FMT "'", STR_ARG(token_names[type]));
        }

        type = va_arg(ap, enum lex_token_type);
        fprintf(stderr, ", ");
    }

    fprintf(stderr, "found '" STR_FMT "'\n", STR_ARG(token_names[token->type]));
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

static struct ast_object* parse_object(
    struct arena* arena, struct lexer* lexer
);

static void
parse_object_copies(
    struct arena* arena, struct lexer* lexer,
    struct ast_object_copy* object_copy
) {
    struct ast_object_copy** next = &object_copy;
    struct token token            = {};
    bool cont                     = true;
    while (cont) {
        switch (lex(arena, lexer, &token)) {
            case TOKEN_DOT: {
                (*next)->next
                    = arena_allocate(arena, sizeof(struct ast_object_copy));
                next                = &(*next)->next;
                struct token token2 = {};
                expect_token(arena, lexer, TOKEN_NAME, &token2);
                (*next)->name = string_duplicate(arena, token2.name);
                break;
            }
            case TOKEN_OPEN_PAREN: {
                (*next)->free_properties = arena_allocate(
                    arena, sizeof(struct ast_free_property_assign)
                );
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
                        = parse_object(arena, lexer);
                    (*next_free_property_assign)->next = arena_allocate(
                        arena, sizeof(struct ast_free_property_assign)
                    );
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
                                = parse_object(arena, lexer);
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
    struct arena* arena, struct lexer* lexer
);

// object ::= (identifier+ ">")? (("{" (property)* "}") | object_copy |
// primitive)
static struct ast_object*
parse_object(struct arena* arena, struct lexer* lexer) {
    struct token token        = {};
    struct ast_object* object = create_object(arena);

    object->properties = create_property(arena, AST_PROPERTY_TYPE_NON_FREE);
    struct ast_property** next = &object->properties;

    enum lex_token_type tt = lex(arena, lexer, &token);
    switch (tt) {
        case TOKEN_NAME: {
            // name can mean both identifiers or object copy
            struct token token2   = {};
            enum lex_token_type t = lex(arena, lexer, &token2);
            switch (t) {
                case TOKEN_NAME: { // more than one free property
                    unlex(lexer, &token2);

                    (*next)->type   = AST_PROPERTY_TYPE_FREE;
                    (*next)->name   = string_duplicate(arena, token.name);
                    (*next)->object = create_object(arena);
                    (*next)->object->alias = AST_OBJECT_ALIAS_ANY;
                    (*next)->next
                        = create_property(arena, AST_PROPERTY_TYPE_FREE);
                    next = &(*next)->next;

                    struct token token3 = {};
                    bool break_out      = false;
                    while (!break_out) {
                        switch (lex(arena, lexer, &token3)) {
                            case TOKEN_NAME:
                                (*next)->type = AST_PROPERTY_TYPE_FREE;
                                (*next)->name
                                    = string_duplicate(arena, token3.name);
                                (*next)->object        = create_object(arena);
                                (*next)->object->alias = AST_OBJECT_ALIAS_ANY;
                                (*next)->next          = create_property(
                                    arena, AST_PROPERTY_TYPE_FREE
                                );
                                next = &(*next)->next;
                                break;
                            case TOKEN_GREATER_THAN:
                                break_out = true;
                                break;
                            default:
                                synerror(
                                    arena, &token, TOKEN_GREATER_THAN,
                                    TOKEN_NAME
                                );
                        }
                    }

                    break;
                }
                case TOKEN_GREATER_THAN: { // only one free property
                    (*next)->type   = AST_PROPERTY_TYPE_FREE;
                    (*next)->name   = string_duplicate(arena, token.name);
                    (*next)->object = create_object(arena);
                    (*next)->object->alias = AST_OBJECT_ALIAS_ANY;
                    (*next)->next
                        = create_property(arena, AST_PROPERTY_TYPE_NON_FREE);
                    next = &(*next)->next;
                    break;
                }
                default: { // object copy
                    object->alias = AST_OBJECT_ALIAS_OBJECT_COPY;
                    unlex(lexer, &token2);
                    struct ast_object_copy* object_copy
                        = arena_allocate(arena, sizeof(struct ast_object_copy));
                    object->object_copy = object_copy;
                    object_copy->name   = string_duplicate(arena, token.name);
                    printf(STR_FMT "\n", STR_ARG(object_copy->name));
                    parse_object_copies(arena, lexer, object_copy);
                    break;
                }
            }
            break;
        }
        default:
            unlex(lexer, &token);
            break;
    }
    if (object->alias == AST_OBJECT_ALIAS_OBJECT_COPY) {
        return object;
    }
    tt = lex(arena, lexer, &token);
    switch (tt) {
        case TOKEN_NAME:
            object->alias = AST_OBJECT_ALIAS_OBJECT_COPY;
            struct ast_object_copy* object_copy
                = arena_allocate(arena, sizeof(struct ast_object_copy));
            object->object_copy = object_copy;
            object_copy->name   = string_duplicate(arena, token.name);
            parse_object_copies(arena, lexer, object_copy);
            break;
        case TOKEN_OPEN_BRACE: {
            struct token token2 = {};
            bool break_out      = false;
            while (!break_out) {
                switch (lex(arena, lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        *next     = nullptr;
                        break_out = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        *next         = parse_property(arena, lexer);
                        (*next)->next = create_property(
                            arena, AST_PROPERTY_TYPE_NON_FREE
                        );
                        next = &(*next)->next;
                        break;
                }
            }
            break;
        }
        case TOKEN_INTEGER:
            object->alias   = AST_OBJECT_ALIAS_INTEGER;
            object->integer = token.integer;
            break;
        case TOKEN_STRING:
            object->alias  = AST_OBJECT_ALIAS_STRING;
            object->string = string_duplicate(arena, token.string);
            break;
        default:
            unlex(lexer, &token);
            break;
    }
    *next = nullptr;

    return object;
}

// property ::= identifier "=" object ";"
static struct ast_property*
parse_property(struct arena* arena, struct lexer* lexer) {
    struct token token       = {};
    enum lex_token_type type = lex(arena, lexer, &token);
    if (type != TOKEN_NAME) {
        unlex(lexer, &token);
        return nullptr;
    }
    struct ast_property* property
        = create_property(arena, AST_PROPERTY_TYPE_NON_FREE);
    property->name = string_duplicate(arena, token.name);

    expect_token(arena, lexer, TOKEN_ASSIGN, nullptr);

    property->object = parse_object(arena, lexer);

    expect_token(arena, lexer, TOKEN_SEMICOLON, nullptr);

    return property;
}

static struct ast_object*
parse_root(struct arena* arena, struct lexer* lexer) {
    struct ast_object* root = create_object(arena);
    //     root->type       = OBJECT_TYPE_PROPERTIES;
    root->properties = arena_allocate(arena, sizeof(struct ast_property));
    struct ast_property** next_property = &root->properties;

    while (true) {
        *next_property = parse_property(arena, lexer);
        if (*next_property == nullptr) {
            break;
        }
        (*next_property)->next
            = arena_allocate(arena, sizeof(struct ast_property));
        next_property = &(*next_property)->next;
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

struct string
property_type_as_string(enum ast_property_type type) {
    switch (type) {
        case AST_PROPERTY_TYPE_FREE:
            return from_cstr("FREE");
        case AST_PROPERTY_TYPE_NON_FREE:
            return from_cstr("NON_FREE");
    }
    vix_unreachable();
}

static void
print_property(struct ast_property* property, usize indent) {
    if (property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Property: " STR_FMT "\n", STR_ARG(property->name));
    print_indent(indent + 2);
    fprintf(
        stdout, STR_FMT "\n", STR_ARG(property_type_as_string(property->type))
    );
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
    fprintf(stdout, "Object Copy: " STR_FMT "\n", STR_ARG(object_copy->name));
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
    // if (object->parent != nullptr) {
    //     fprintf(stdout, ", parent %d", object->parent->id);
    // }
    fprintf(stdout, ":\n");
    print_property(object->properties, indent + 2);
    switch (object->alias) {
        case AST_OBJECT_ALIAS_NONE:
            break;
        case AST_OBJECT_ALIAS_INTEGER:
            print_indent(indent);
            fprintf(stdout, "  alias: INTEGER\n");
            print_indent(indent);
            fprintf(stdout, "  value: %ld\n", object->integer);
            break;
        case AST_OBJECT_ALIAS_STRING:
            print_indent(indent);
            fprintf(stdout, "  alias: STRING\n");
            print_indent(indent);
            fprintf(stdout, "  value: " STR_FMT "\n", STR_ARG(object->string));
            break;
        case AST_OBJECT_ALIAS_ANY:
            print_indent(indent);
            fprintf(stdout, "  alias: ANY\n");
            break;
        case AST_OBJECT_ALIAS_OBJECT_COPY:
            print_object_copy(object->object_copy, indent + 2);
            break;
            break;
    }
    // print_free_property(object->free_properties, indent + 2);
    // switch (object->type) {
    //     case OBJECT_TYPE_OBJECT_COPY:
    //         print_object_copy(object->object_copy, indent + 2);
    //         break;
    //     case OBJECT_TYPE_PROPERTIES:
    //         print_property(object->properties, indent + 2);
    //         break;
    //     case OBJECT_TYPE_INTEGER:
    //         print_indent(indent);
    //         fprintf(stdout, "  value: %ld\n", object->integer);
    //         break;
    //     case OBJECT_TYPE_STRING:
    //         print_indent(indent);
    //         fprintf(stdout, "  value: " STR_FMT "\n",
    //         STR_ARG(object->string)); break;
    //     case OBJECT_TYPE_NONE:
    //         break;
    // }
}
