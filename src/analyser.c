#include "analyser.h"

#include "array.h"
#include "error_message.h"
#include "parser.h"
#include "str.h"
#include "util.h"

static struct error_message_t* add_node_error(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const node[static const 1], struct str_t message
) {
    struct error_message_t error_message = error_message_create_with_line(
        code_gen->allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    struct error_message_t* err = code_gen->allocator->allocate(
        sizeof(struct error_message_t), code_gen->allocator->context
    );
    memcpy(err, &error_message, sizeof(struct error_message_t));

    array_push(code_gen->errors, err);

    return err;
}

static void add_error_note(
    struct code_gen_t code_gen[static const 1],
    struct error_message_t parent_message[static const 1],
    struct ast_node_t const node[static const 1], struct str_t message
) {
    struct error_message_t error_message = error_message_create_with_line(
        code_gen->allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    struct error_message_t* err = code_gen->allocator->allocate(
        sizeof(struct error_message_t), code_gen->allocator->context
    );
    memcpy(err, &error_message, sizeof(struct error_message_t));

    error_message_add_note(parent_message, err);
}

static struct ast_node_t const* find_identifier(
    struct scope_t const scope[static const 1], struct str_t identifier
) {
    for (size_t i = 0; i < array_length_unsigned(scope->properties); ++i) {
        if (str_equal(
                scope->properties[i]->data.identifier.content, identifier
            )) {
            return scope->properties[i];
        }
    }
    return nullptr;
}

static void analyse_node(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const node[static const 1]
);

static void analyse_property(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const property[static const 1]
);

static void analyse_identifier(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const identifier[static const 1]
) {
    struct str_t identifier_content = identifier->data.identifier.content;
    struct ast_node_t const* existing_identifier =
        find_identifier(scope, identifier_content);
    if (existing_identifier) {
        struct str_buffer_t buffer = str_buffer_new(code_gen->allocator, 0);
        str_buffer_printf(
            &buffer, str_new("redefinition of '%s'"), identifier_content
        );
        struct error_message_t* error_message =
            add_node_error(code_gen, identifier, str_buffer_str(&buffer));
        str_buffer_printf(&buffer, str_new("previous definition is here"));
        add_error_note(
            code_gen, error_message, existing_identifier,
            str_buffer_str(&buffer)
        );
    } else {
        array_push(scope->properties, identifier);
    }
}

static void analyse_root(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const root[static const 1]
) {
    for (size_t i = 0; i < array_length_unsigned(root->data.root.list); ++i) {
        analyse_node(code_gen, scope, root->data.root.list[i]);
    }
}

static void analyse_object(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const object[static const 1]
) {
    struct scope_t new_scope = {
        .parent      = scope,
        .source_node = object,
    };
    new_scope.properties = array(struct ast_node_t*, code_gen->allocator);

    for (size_t i = 0; i < array_length_unsigned(object->data.object.free_list);
         ++i) {
        analyse_identifier(
            code_gen, &new_scope, object->data.object.free_list[i]
        );
    }
    for (size_t i = 0;
         i < array_length_unsigned(object->data.object.property_list); ++i) {
        struct ast_node_t* node = object->data.object.property_list[i];
        if (node->type == NODE_TYPE_PROPERTY) {
            analyse_property(
                code_gen, &new_scope, object->data.object.property_list[i]
            );
        } else if (node->type == NODE_TYPE_DECORATOR) {
            // TODO
        } else {
            vix_unreachable();
        }
    }
}

static void analyse_property(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const property[static const 1]
) {
    assert(
        property->data.property.identifier->data.identifier.content.length != 0
    );
    analyse_identifier(code_gen, scope, property->data.property.identifier);
    analyse_node(code_gen, scope, property->data.property.object_result);
}

static void analyse_node(
    struct code_gen_t code_gen[static const 1],
    struct scope_t scope[static const 1],
    struct ast_node_t const node[static const 1]
) {
    switch (node->type) {
    case NODE_TYPE_ROOT:
        analyse_root(code_gen, scope, node);
        break;
    case NODE_TYPE_PROPERTY:
        analyse_property(code_gen, scope, node);
        break;
    case NODE_TYPE_OBJECT:
        analyse_object(code_gen, scope, node);
        break;
    case NODE_TYPE_STRING_LITERAL:
    case NODE_TYPE_CHAR_LITERAL:
    case NODE_TYPE_INTEGER:
    case NODE_TYPE_DECORATOR:
    case NODE_TYPE_IDENTIFIER:
    case NODE_TYPE_FREE_OBJECT_COPY:
    case NODE_TYPE_OBJECT_PROPERTY_ACCESS:
        break;
    }
}

static void report_errors_and_maybe_exit(
    struct code_gen_t const code_gen[static const 1]
) {
    if (array_length_unsigned(code_gen->errors) > 0) {
        for (size_t i = 0; i < array_length_unsigned(code_gen->errors); ++i) {
            struct error_message_t* error_message = code_gen->errors[i];
            print_error_message(error_message, code_gen->error_color);
        }
        exit(1);
    }
}

void analyse(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const root[static const 1]
) {
    analyse_node(code_gen, &code_gen->root_scope, root);

    report_errors_and_maybe_exit(code_gen);
}

static void write_program_start(struct str_buffer_t buffer[static const 1]) {
    str_buffer_reset(buffer);
    str_buffer_append_printf(
        buffer, str_new("global  _start\n"
                        "_start:\n"
                        "\n")
    );
}

static void write_program_end(struct str_buffer_t buffer[static const 1]) {
    str_buffer_append_printf(
        buffer,
        str_new(
            "_end:\n"
            "    mov     ebx, 0      ; return 0 status on exit - 'No Errors'\n"
            "    mov     eax, 1      ; invoke SYS_EXIT (kernel opcode 1)\n"
            "    int     80h\n"
        )
    );
}

static void generate_node(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const node[static const 1],
    struct str_buffer_t prefix[static const 1],
    struct str_buffer_t buffer[static const 1]
) {
    switch (node->type) {
    case NODE_TYPE_ROOT:
        for (size_t i = 0; i < array_length_unsigned(node->data.root.list);
             ++i) {
            generate_node(code_gen, node->data.root.list[i], prefix, buffer);
        }
        break;
    case NODE_TYPE_PROPERTY: {
        struct str_buffer_t new_prefix = str_buffer_new(prefix->allocator, 0);
        str_buffer_append_printf(
            &new_prefix, str_new(str_fmt "_" str_fmt),
            str_args(*prefix),
            str_args(node->data.property.identifier->data.identifier.content)
        );
        printf("a "str_fmt"\n", str_args(*prefix));
        printf("b "str_fmt"\n", str_args(node->data.property.identifier->data.identifier.content));
        printf("c "str_fmt"\n", str_args(new_prefix));
        str_buffer_append_printf(
            buffer, str_new(str_fmt ":\n"), str_args(new_prefix)
        );
        generate_node(
            code_gen, node->data.property.object_result, &new_prefix, buffer
        );
    } break;
    case NODE_TYPE_OBJECT:
        // TODE free properties
        for (size_t i = 0;
             i < array_length_unsigned(node->data.object.property_list); ++i) {
            generate_node(
                code_gen, node->data.object.property_list[i], prefix, buffer
            );
        }
    case NODE_TYPE_STRING_LITERAL:
    case NODE_TYPE_CHAR_LITERAL:
    case NODE_TYPE_INTEGER:
    case NODE_TYPE_DECORATOR:
    case NODE_TYPE_IDENTIFIER:
    case NODE_TYPE_FREE_OBJECT_COPY:
    case NODE_TYPE_OBJECT_PROPERTY_ACCESS:
        break;
    }
}

struct str_t generate(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const root[static const 1]
) {
    struct str_buffer_t buffer = str_buffer_new(code_gen->allocator, 0);

    write_program_start(&buffer);

    struct str_buffer_t prefix = str_buffer_new(code_gen->allocator, 5);
    str_buffer_append(&prefix, str_new("_root"));

    generate_node(code_gen, root, &prefix, &buffer);

    write_program_end(&buffer);

    return str_buffer_str(&buffer);
}
