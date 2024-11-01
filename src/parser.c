#include "parser.h"

#include "allocator.h"
#include "ast.h"
#include "lexer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdnoreturn.h>

struct parser_entry* buckets[BUCKET_SIZE];

static void
buckets_insert_object(struct arena* arena, struct ast_object* object) {
    struct parser_entry* entry
        = arena_allocate(arena, sizeof(struct parser_entry));
    entry->type   = PARSER_ENTRY_TYPE_OBJECT;
    entry->object = object;

    struct parser_entry** bucket = &buckets[object->id % BUCKET_SIZE];
    if (*bucket != nullptr) {
        entry->next = *bucket;
    }
    *bucket = entry;
}

static void
buckets_insert_property(struct arena* arena, struct ast_property* property) {
    struct parser_entry* entry
        = arena_allocate(arena, sizeof(struct parser_entry));
    entry->type     = PARSER_ENTRY_TYPE_PROPERTY;
    entry->property = property;

    struct parser_entry** bucket = &buckets[property->id % BUCKET_SIZE];
    if (*bucket != nullptr) {
        entry->next = *bucket;
    }
    *bucket = entry;
}

static u32 id = 0;

static struct ast_object*
create_object(struct arena* arena, struct ast_object* parent) {
    struct ast_object* new_object
        = arena_allocate(arena, sizeof(struct ast_object));
    new_object->type.type   = AST_STYPE_OBJECT;
    new_object->object_copy = nullptr;
    new_object->parent      = parent;
    new_object->id          = id;
    new_object->type.id     = id;
    id += 1;
    buckets_insert_object(arena, new_object);
    return new_object;
}

static struct ast_property*
add_property(
    struct arena* arena, struct ast_object* object, enum ast_property_type type
) {
    struct ast_property* new_property
        = arena_allocate(arena, sizeof(struct ast_property));
    new_property->type = type;
    new_property->id   = id;
    id += 1;
    buckets_insert_property(arena, new_property);

    struct ast_property* property = object->properties;
    if (property == nullptr) {
        object->properties = new_property;
    } else {
        while (property != nullptr) {
            if (property->next == nullptr) {
                break;
            }
            property = property->next;
        }
        property->next = new_property;
    }
    return new_property;
}

static struct ast_object_property_type*
add_type_object_type(struct arena* arena, struct ast_object* object) {
    struct ast_object_property_type* new_type
        = arena_allocate(arena, sizeof(struct ast_object_property_type));
    new_type->type = arena_allocate(arena, sizeof(struct ast_type));

    struct ast_object_property_type* type = object->type.object_types;
    if (type == nullptr) {
        object->type.object_types = new_type;
    } else {
        while (type != nullptr) {
            if (type->next == nullptr) {
                break;
            }
            type = type->next;
        }
        type->next = new_type;
    }
    return new_type;
}

static struct ast_object_copy*
add_object_copy(struct arena* arena, struct ast_object* object) {
    struct ast_object_copy* new_object_copy
        = arena_allocate(arena, sizeof(struct ast_object_copy));

    struct ast_object_copy* object_copy = object->object_copy;
    if (object_copy == nullptr) {
        object->object_copy = new_object_copy;
    } else {
        while (object_copy != nullptr) {
            if (object_copy->next == nullptr) {
                break;
            }
            object_copy = object_copy->next;
        }
        object_copy->next = new_object_copy;
    }
    return new_object_copy;
}

static struct ast_free_property_assign*
add_free_property_assign(
    struct arena* arena, struct ast_object_copy* object_copy
) {
    struct ast_free_property_assign* new_free_property_assign
        = arena_allocate(arena, sizeof(struct ast_free_property_assign));

    struct ast_free_property_assign* free_property_assign
        = object_copy->free_properties;
    if (free_property_assign == nullptr) {
        object_copy->free_properties = new_free_property_assign;
    } else {
        while (free_property_assign != nullptr) {
            if (free_property_assign->next == nullptr) {
                break;
            }
            free_property_assign = free_property_assign->next;
        }
        free_property_assign->next = new_free_property_assign;
    }
    return new_free_property_assign;
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
    struct arena* arena, struct lexer* lexer, struct ast_object* parent
);

static void
parse_object_copies(
    struct arena* arena, struct lexer* lexer, struct ast_object* object,
    struct ast_object_copy* object_copy, struct ast_object* parent
) {
    struct token token = {};
    bool cont          = true;
    while (cont) {
        switch (lex(arena, lexer, &token)) {
            case TOKEN_DOT: {
                object_copy         = add_object_copy(arena, object);
                struct token token2 = {};
                expect_token(arena, lexer, TOKEN_NAME, &token2);
                object_copy->name = string_duplicate(arena, token2.name);
                break;
            }
            case TOKEN_OPEN_PAREN: {
                struct token token2      = {};
                enum lex_token_type type = lex(arena, lexer, &token2);
                if (type == TOKEN_CLOSE_PAREN) {
                    break;
                } else {
                    unlex(lexer, &token2);
                    struct ast_free_property_assign* next_free_property_assign
                        = add_free_property_assign(arena, object_copy);
                    next_free_property_assign->value
                        = parse_object(arena, lexer, parent);
                }
                bool break_out = false;
                while (!break_out) {
                    switch (lex(arena, lexer, &token2)) {
                        case TOKEN_CLOSE_PAREN:
                            break_out = true;
                            break;
                        case TOKEN_COMMA:
                            struct ast_free_property_assign*
                                next_free_property_assign
                                = add_free_property_assign(arena, object_copy);
                            next_free_property_assign->value
                                = parse_object(arena, lexer, parent);
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
    struct ast_object* object = create_object(arena, parent);

    switch (lex(arena, lexer, &token)) {
        case TOKEN_NAME: {
            // name can mean both identifiers or object copy
            struct token token2   = {};
            enum lex_token_type t = lex(arena, lexer, &token2);
            switch (t) {
                case TOKEN_NAME: { // more than one free property
                    unlex(lexer, &token2);

                    struct ast_property* next = add_property(
                        arena, object, AST_PROPERTY_TYPE_NON_FREE
                    );
                    next->type   = AST_PROPERTY_TYPE_FREE;
                    next->name   = string_duplicate(arena, token.name);
                    next->object = create_object(arena, object);
                    next->object->type.type = AST_STYPE_ANY;

                    struct ast_object_property_type* next_type
                        = add_type_object_type(arena, object);

                    next_type->name       = string_duplicate(arena, token.name);
                    next_type->type->type = AST_STYPE_ALIAS;
                    next_type->type->alias_type = &next->object->type;

                    struct token token3 = {};
                    bool break_out      = false;
                    while (!break_out) {
                        switch (lex(arena, lexer, &token3)) {
                            case TOKEN_NAME:
                                struct ast_property* next = add_property(
                                    arena, object, AST_PROPERTY_TYPE_NON_FREE
                                );
                                next->type = AST_PROPERTY_TYPE_FREE;
                                next->name
                                    = string_duplicate(arena, token3.name);
                                next->object = create_object(arena, object);
                                next->object->type.type = AST_STYPE_ANY;

                                struct ast_object_property_type* next_type
                                    = add_type_object_type(arena, object);

                                next_type->name
                                    = string_duplicate(arena, token3.name);
                                next_type->type->type = AST_STYPE_ALIAS;
                                next_type->type->alias_type
                                    = &next->object->type;

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
                    struct ast_property* next = add_property(
                        arena, object, AST_PROPERTY_TYPE_NON_FREE
                    );
                    next->type   = AST_PROPERTY_TYPE_FREE;
                    next->name   = string_duplicate(arena, token.name);
                    next->object = create_object(arena, object);
                    next->object->type.type = AST_STYPE_ANY;

                    struct ast_object_property_type* next_type
                        = add_type_object_type(arena, object);

                    next_type->name       = string_duplicate(arena, token.name);
                    next_type->type->type = AST_STYPE_ALIAS;
                    next_type->type->alias_type = &next->object->type;
                    break;
                }
                default: { // object copy
                    object->type.type = AST_STYPE_COPY;
                    unlex(lexer, &token2);
                    add_object_copy(arena, object);
                    object->object_copy->name
                        = string_duplicate(arena, token.name);
                    parse_object_copies(
                        arena, lexer, object, object->object_copy, object
                    );
                    break;
                }
            }
            break;
        }
        default:
            unlex(lexer, &token);
            break;
    }
    if (object->type.type == AST_STYPE_COPY) {
        return object;
    }
    switch (lex(arena, lexer, &token)) {
        case TOKEN_NAME:
            object->type.type = AST_STYPE_COPY;
            add_object_copy(arena, object);
            object->object_copy->name = string_duplicate(arena, token.name);
            parse_object_copies(
                arena, lexer, object, object->object_copy, object
            );
            break;
        case TOKEN_OPEN_BRACE: {
            struct token token2 = {};
            bool break_out      = false;
            while (!break_out) {
                switch (lex(arena, lexer, &token2)) {
                    case TOKEN_CLOSE_BRACE:
                        break_out = true;
                        break;
                    default:
                        unlex(lexer, &token2);
                        struct ast_property* next
                            = parse_property(arena, lexer, object);

                        struct ast_object_property_type* next_type
                            = add_type_object_type(arena, object);

                        next_type->name = string_duplicate(arena, next->name);
                        next_type->type->type       = AST_STYPE_ALIAS;
                        next_type->type->alias_type = &next->object->type;
                        break;
                }
            }
            break;
        }
        case TOKEN_INTEGER:
            object->type.type       = AST_STYPE_OBJECT;
            object->type.extra_type = AST_EXTRA_STYPE_INTEGER;
            object->integer         = token.integer;
            break;
        case TOKEN_STRING:
            object->type.type       = AST_STYPE_OBJECT;
            object->type.extra_type = AST_EXTRA_STYPE_STRING;
            object->string          = string_duplicate(arena, token.string);
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
    struct ast_property* property
        = add_property(arena, parent, AST_PROPERTY_TYPE_NON_FREE);
    property->name = string_duplicate(arena, token.name);

    expect_token(arena, lexer, TOKEN_ASSIGN, nullptr);

    property->object = parse_object(arena, lexer, parent);

    expect_token(arena, lexer, TOKEN_SEMICOLON, nullptr);

    return property;
}

static struct ast_object*
parse_root(struct arena* arena, struct lexer* lexer) {
    struct ast_object* root = create_object(arena, nullptr);

    while (true) {
        struct ast_property* property = parse_property(arena, lexer, root);
        if (property == nullptr) {
            break;
        }
        struct ast_object_property_type* type
            = add_type_object_type(arena, root);
        type->name             = string_duplicate(arena, property->name);
        type->type->type       = AST_STYPE_ALIAS;
        type->type->alias_type = &property->object->type;
    }

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
print_property(struct ast_property* property, usize indent, bool print_free) {
    if (property == nullptr) {
        return;
    }
    if ((print_free && property->type == AST_PROPERTY_TYPE_FREE)
        || (!print_free && property->type == AST_PROPERTY_TYPE_NON_FREE)) {
        print_indent(indent);
        fprintf(
            stdout, "Property %d: " STR_FMT "\n", property->id,
            STR_ARG(property->name)
        );
        print_indent(indent + 2);
        fprintf(
            stdout, STR_FMT "\n",
            STR_ARG(property_type_as_string(property->type))
        );
        print_object(property->object, indent + 2);
    }
    print_property(property->next, indent, print_free);
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
print_type(struct ast_type* type, usize indent) {
    switch (type->type) {
        case AST_STYPE_ANY:
            fprintf(stdout, "ANY");
            break;
        case AST_STYPE_COPY:
            fprintf(stdout, "COPY");
            break;
        case AST_STYPE_OBJECT:
            fprintf(stdout, "OBJECT {\n");
            struct ast_object_property_type* object_type = type->object_types;
            while (object_type != nullptr) {
                print_indent(indent + 2);
                fprintf(stdout, STR_FMT ": ", STR_ARG(object_type->name));
                print_type(object_type->type, indent + 2);
                fprintf(stdout, "\n");
                object_type = object_type->next;
            }
            print_indent(indent);
            fprintf(stdout, "}");
            switch (type->extra_type) {
                case AST_EXTRA_STYPE_INTEGER:
                    fprintf(stdout, "\n");
                    print_indent(indent);
                    fprintf(stdout, "extra: INTEGER");
                    break;
                case AST_EXTRA_STYPE_STRING:
                    fprintf(stdout, "\n");
                    print_indent(indent);
                    fprintf(stdout, "extra: STRING");
                    break;
                case AST_EXTRA_STYPE_NONE:
                    break;
            }
            break;
        case AST_STYPE_UNION:
            fprintf(stdout, "UNION {\n");
            struct ast_union_type* union_type = type->union_type;
            while (union_type != nullptr) {
                print_indent(indent + 2);
                fprintf(stdout, "(");
                print_type(union_type->type, indent + 2);
                fprintf(stdout, "),\n");

                union_type = union_type->next;
            }
            print_indent(indent);
            fprintf(stdout, "}");
            break;
        case AST_STYPE_ALIAS:
            fprintf(stdout, "ALIAS %d", type->alias_type->id);
            break;
    }
    fprintf(stdout, "\n");
    print_indent(indent);
    fprintf(stdout, "size: %d", type->size);
}

void
print_object(struct ast_object* object, usize indent) {
    if (object == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(stdout, "Object %d", object->id);
    if (object->parent != nullptr) {
        fprintf(stdout, ", parent: %d", object->parent->id);
    }
    fprintf(stdout, ":\n");

    print_indent(indent + 4);
    fprintf(stdout, "type ");
    print_type(&object->type, indent + 4);
    fprintf(stdout, "\n");

    print_property(object->properties, indent + 2, true);

    switch (object->type.type) {
        case AST_STYPE_ANY:
        case AST_STYPE_ALIAS:
        case AST_STYPE_UNION:
            break;
        case AST_STYPE_COPY:
            print_object_copy(object->object_copy, indent + 2);
            break;
        case AST_STYPE_OBJECT:
            switch (object->type.extra_type) {
                case AST_EXTRA_STYPE_INTEGER:
                    print_indent(indent);
                    fprintf(stdout, "  value: %ld\n", object->integer);
                    break;
                case AST_EXTRA_STYPE_STRING:
                    print_indent(indent);
                    fprintf(
                        stdout, "  value: " STR_FMT "\n",
                        STR_ARG(object->string)
                    );
                    break;
                case AST_EXTRA_STYPE_NONE:
                    break;
            }
            print_property(object->properties, indent + 2, false);
            break;
    }
}
