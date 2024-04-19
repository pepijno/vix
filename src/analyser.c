#include "analyser.h"

#include "array.h"
#include "error_message.h"
#include "parser.h"
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
    analyse_node(code_gen, scope, property->data.property.property_value);
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
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
        case NODE_TYPE_OBJECT_COPY:
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

size_t global_id = 0;

static void write_program_start(
    struct str_buffer_t buffer[static const 1],
    struct ast_node_t const root[static const 1]
) {
    str_buffer_reset(buffer);
    str_buffer_append(buffer, str_new("#include <stdio.h>\n\n"));
}

static void write_program_end(
    struct code_gen_t code_gen[static const 1],
    struct str_buffer_t buffer[static const 1]
) {
}

static struct str_t property_return_type(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const node[static const 1]
) {
    assert(node->type == NODE_TYPE_PROPERTY);

    if (node->data.property.property_value->type == NODE_TYPE_CHAR_LITERAL) {
        return str_new("char");
    } else if (node->data.property.property_value->type == NODE_TYPE_INTEGER) {
        return str_new("int");
    } else if (node->data.property.property_value->type == NODE_TYPE_STRING_LITERAL) {
        return str_new("char*");
    } else if (node->data.property.property_value->type == NODE_TYPE_OBJECT) {
        struct str_buffer_t type_buffer =
            str_buffer_new(code_gen->allocator, 0);
        str_buffer_printf(
            &type_buffer, str_new("struct " str_fmt "_return"),
            str_args(node->data.property.identifier->data.identifier.content)
        );
        struct str_t t = str_buffer_str(&type_buffer);
        return t;
    } else {
        return str_new("int");
        // printf("%d " str_fmt "\n", node->data.property.property_value->type,
        // str_args(node->data.property.identifier->data.identifier.content));
        // assert(false);
    }
    vix_unreachable();
}

static void generate_node(
    struct code_gen_t code_gen[static const 1],
    struct ast_node_t const node[static const 1],
    struct str_buffer_t buffer[static const 1], struct str_t prefix, int j
) {
    switch (node->type) {
        case NODE_TYPE_ROOT: {
            for (size_t i = 0; i < array_length_unsigned(node->data.root.list);
                 ++i) {
                generate_node(
                    code_gen, node->data.root.list[i], buffer, prefix, j + 1
                );
            }
            break;
        }
        case NODE_TYPE_STRING_LITERAL:
            str_buffer_append_printf(
                buffer, str_new("\"" str_fmt "\""),
                str_args(node->data.string_literal.content)
            );
            break;
        case NODE_TYPE_CHAR_LITERAL:
            str_buffer_append_printf(
                buffer, str_new("'%c'"), node->data.char_literal.c
            );
            break;
        case NODE_TYPE_INTEGER:
            str_buffer_append(buffer, node->data.integer.content);
            break;
        case NODE_TYPE_OBJECT: {
            break;
        }
        case NODE_TYPE_PROPERTY: {
            struct str_t type = property_return_type(code_gen, node);
            if (node->data.property.property_value->type == NODE_TYPE_OBJECT) {
                for (size_t i = 0;
                     i <
                     array_length_unsigned(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    struct ast_node_t const* const n =
                        node->data.property.property_value->data.object
                            .property_list[i];
                    generate_node(code_gen, n, buffer, prefix, j + 1);
                }
                str_buffer_append_printf(
                    buffer, str_new("" str_fmt " {\n"), str_args(type)
                );
                for (size_t i = 0;
                     i <
                     array_length_unsigned(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    struct ast_node_t const* const n =
                        node->data.property.property_value->data.object
                            .property_list[i];
                    struct str_t child_type = property_return_type(code_gen, n);
                    str_buffer_append_printf(
                        buffer,
                        str_new(""
                                "    " str_fmt " (*" str_fmt ")("),
                        str_args(child_type),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        )
                    );
                    str_buffer_append(buffer, str_new(");\n"));
                }

                str_buffer_append(buffer, str_new("};\n"));

                str_buffer_append_printf(
                    buffer, str_new("" str_fmt " " str_fmt "("), str_args(type),
                    str_args(
                        node->data.property.identifier->data.identifier.content
                    )
                );
                for (size_t i = 0; i < array_length_unsigned(node->data.property.property_value->data.object.free_list); ++i) {
                    if (i != 0) {
                        str_buffer_append(buffer, str_new(", "));
                    }
                    str_buffer_append_printf(buffer, str_new("int param%ld"), i);
                    // struct ast_node_t* n = node->data.property.property_value->data.object.free_list[i];
                }
                str_buffer_append(buffer, str_new(") {\n"));
                str_buffer_append_printf(
                    buffer, str_new("    return (" str_fmt "){\n"),
                    str_args(type)
                );
                for (size_t i = 0;
                     i <
                     array_length_unsigned(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    struct ast_node_t const* const n =
                        node->data.property.property_value->data.object
                            .property_list[i];
                    str_buffer_append_printf(
                        buffer,
                        str_new("        ." str_fmt " = " str_fmt ",\n"),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        ),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        )
                    );
                }
                str_buffer_append_printf(
                    buffer, str_new("    };\n"
                                    "}\n")
                );
            } else {
                str_buffer_append_printf(
                    buffer,
                    str_new(str_fmt " " str_fmt "(void) {\n    return "),
                    str_args(type),
                    str_args(
                        node->data.property.identifier->data.identifier.content
                    )
                );

                generate_node(
                    code_gen, node->data.property.property_value, buffer,
                    prefix, j + 1
                );

                str_buffer_append_printf(buffer, str_new(";\n}\n"));
            }
            break;
        }
        case NODE_TYPE_IDENTIFIER:
            break;
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS: {
            str_buffer_append_char(buffer, '(');
            for (size_t i = 0;
                 i < array_length_unsigned(
                         node->data.free_object_copy_params.parameter_list
                     );
                 ++i) {
                if (i != 0) {
                    str_buffer_append(buffer, str_new(", "));
                }
                struct ast_node_t* n =
                    node->data.free_object_copy_params.parameter_list[i];
                generate_node(code_gen, n, buffer, prefix, j + 1);
            }
            str_buffer_append_char(buffer, ')');
            break;
        }
        case NODE_TYPE_OBJECT_COPY: {
            struct str_t identifier =
                node->data.object_copy.identifier->data.identifier.content;
            if (str_equal(identifier, str_new("print"))) {
                assert(
                    array_length_unsigned(
                        node->data.object_copy.free_object_copies
                    ) == 1
                );
                assert(node->data.object_copy.object_copy == nullptr);
                struct ast_node_t* copies =
                    node->data.object_copy.free_object_copies[0];
                assert(
                    array_length_unsigned(
                        copies->data.free_object_copy_params.parameter_list
                    ) == 1
                );
                str_buffer_append_printf(
                    buffer, str_new("printf(\"%%d\", " str_fmt ")"),
                    str_args(copies->data.free_object_copy_params
                                 .parameter_list[0]
                                 ->data.integer.content)
                );
            } else {
                str_buffer_append_printf(
                    buffer, str_new(str_fmt),
                    str_args(node->data.object_copy.identifier->data.identifier
                                 .content)
                );
                if (array_length_unsigned(
                        node->data.object_copy.free_object_copies
                    ) != 0) {
                    for (size_t i = 0;
                         i < array_length_unsigned(
                                 node->data.object_copy.free_object_copies
                             );
                         ++i) {
                        struct ast_node_t* n =
                            node->data.object_copy.free_object_copies[i];
                        generate_node(code_gen, n, buffer, prefix, j + 1);
                    }
                } else {
                    str_buffer_append(buffer, str_new("()"));
                }
                if (node->data.object_copy.object_copy != nullptr) {
                    str_buffer_append(buffer, str_new("."));
                    generate_node(
                        code_gen, node->data.object_copy.object_copy, buffer,
                        prefix, j + 1
                    );
                }
            }
            break;
        }
        case NODE_TYPE_DECORATOR:
            break;
    }
}

void generate(
    struct code_gen_t code_gen[static const 1],
    struct str_buffer_t buffer[static const 1],
    struct ast_node_t const root[static const 1]
) {
    write_program_start(buffer, root);

    generate_node(code_gen, root, buffer, str_new("_root"), 0);

    write_program_end(code_gen, buffer);
}
