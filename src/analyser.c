#include "analyser.h"

#include "array.h"
#include "error_message.h"
#include "parser.h"
#include "util.h"

static ErrorMessage* add_node_error(
    CodeGen code_gen[static 1], AstNode node[static 1], Str message
) {
    ErrorMessage error_message = error_message_create_with_line(
        code_gen->allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    ErrorMessage* err = code_gen->allocator->allocate(
        sizeof(ErrorMessage), code_gen->allocator->context
    );
    memcpy(err, &error_message, sizeof(ErrorMessage));

    array_push(code_gen->errors, err);

    return err;
}

static void add_error_note(
    CodeGen code_gen[static 1], ErrorMessage parent_message[static 1],
    AstNode node[static 1], Str message
) {
    ErrorMessage error_message = error_message_create_with_line(
        code_gen->allocator, node->owner->path, node->line, node->column,
        node->owner->source_code, node->owner->line_offsets, message
    );

    ErrorMessage* err = code_gen->allocator->allocate(
        sizeof(ErrorMessage), code_gen->allocator->context
    );
    memcpy(err, &error_message, sizeof(ErrorMessage));

    error_message_add_note(parent_message, err);
}

static AstNode* find_identifier(Scope scope[static 1], Str identifier) {
    for (i64 i = 0; i < array_length(scope->properties); ++i) {
        if (str_equal(
                scope->properties[i]->data.identifier.content, identifier
            )) {
            return scope->properties[i];
        }
    }
    return nullptr;
}

static void analyse_node(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode node[static 1]
);

static void analyse_property(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode property[static 1]
);

static void analyse_identifier(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode identifier[static 1]
) {
    Str identifier_content = identifier->data.identifier.content;
    AstNode* existing_identifier =
        find_identifier(scope, identifier_content);
    if (existing_identifier) {
        StrBuffer buffer = str_buffer_new(code_gen->allocator, 0);
        str_buffer_printf(
            &buffer, str_new((u8*)"redefinition of '%s'"), identifier_content
        );
        ErrorMessage* error_message =
            add_node_error(code_gen, identifier, str_buffer_str(&buffer));
        str_buffer_printf(&buffer, str_new((u8*)"previous definition is here"));
        add_error_note(
            code_gen, error_message, existing_identifier,
            str_buffer_str(&buffer)
        );
    } else {
        array_push(scope->properties, identifier);
    }
}

static void analyse_root(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode root[static 1]
) {
    for (i64 i = 0; i < array_length(root->data.root.list); ++i) {
        analyse_node(code_gen, scope, root->data.root.list[i]);
    }
}

static void analyse_object(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode object[static 1]
) {
    Scope new_scope = {
        .parent      = scope,
        .source_node = object,
    };
    new_scope.properties = array(AstNode*, code_gen->allocator);

    for (i64 i = 0; i < array_length(object->data.object.free_list);
         ++i) {
        analyse_identifier(
            code_gen, &new_scope, object->data.object.free_list[i]
        );
    }
    for (i64 i = 0;
         i < array_length(object->data.object.property_list); ++i) {
        AstNode* node = object->data.object.property_list[i];
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
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode property[static 1]
) {
    assert(
        property->data.property.identifier->data.identifier.content.length != 0
    );
    analyse_identifier(code_gen, scope, property->data.property.identifier);
    analyse_node(code_gen, scope, property->data.property.property_value);
}

static void analyse_node(
    CodeGen code_gen[static 1], Scope scope[static 1],
    AstNode node[static 1]
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

static void report_errors_and_maybe_exit(CodeGen code_gen[static 1]) {
    if (array_length(code_gen->errors) > 0) {
        for (i64 i = 0; i < array_length(code_gen->errors); ++i) {
            ErrorMessage* error_message = code_gen->errors[i];
            print_error_message(error_message, code_gen->error_color);
        }
        exit(1);
    }
}

void analyse(CodeGen code_gen[static 1], AstNode root[static 1]) {
    analyse_node(code_gen, &code_gen->root_scope, root);

    report_errors_and_maybe_exit(code_gen);
}

i64 global_id = 0;

static void write_program_start(
    StrBuffer buffer[static 1], AstNode root[static 1]
) {
    str_buffer_reset(buffer);
    str_buffer_append(buffer, str_new((u8*)"#include <stdio.h>\n\n"));
}

static void write_program_end(
    CodeGen code_gen[static 1], StrBuffer buffer[static 1]
) {
}

static Str property_return_type(
    CodeGen code_gen[static 1], AstNode node[static 1]
) {
    assert(node->type == NODE_TYPE_PROPERTY);

    if (node->data.property.property_value->type == NODE_TYPE_CHAR_LITERAL) {
        return str_new((u8*)"char");
    } else if (node->data.property.property_value->type == NODE_TYPE_INTEGER) {
        return str_new((u8*)"int");
    } else if (node->data.property.property_value->type == NODE_TYPE_STRING_LITERAL) {
        return str_new((u8*)"char*");
    } else if (node->data.property.property_value->type == NODE_TYPE_OBJECT) {
        StrBuffer type_buffer = str_buffer_new(code_gen->allocator, 0);
        str_buffer_printf(
            &type_buffer, str_new((u8*)"struct " str_fmt "_return"),
            str_args(node->data.property.identifier->data.identifier.content)
        );
        Str t = str_buffer_str(&type_buffer);
        return t;
    } else {
        return str_new((u8*)"int");
        // printf("%d " str_fmt "\n", node->data.property.property_value->type,
        // str_args(node->data.property.identifier->data.identifier.content));
        // assert(false);
    }
    vix_unreachable();
}

static void generate_node(
    CodeGen code_gen[static 1], AstNode node[static 1],
    StrBuffer buffer[static 1], Str prefix, int j
) {
    switch (node->type) {
        case NODE_TYPE_ROOT: {
            for (i64 i = 0; i < array_length(node->data.root.list);
                 ++i) {
                generate_node(
                    code_gen, node->data.root.list[i], buffer, prefix, j + 1
                );
            }
            break;
        }
        case NODE_TYPE_STRING_LITERAL:
            str_buffer_append_printf(
                buffer, str_new((u8*)"\"" str_fmt "\""),
                str_args(node->data.string_literal.content)
            );
            break;
        case NODE_TYPE_CHAR_LITERAL:
            str_buffer_append_printf(
                buffer, str_new((u8*)"'%c'"), node->data.char_literal.c
            );
            break;
        case NODE_TYPE_INTEGER:
            str_buffer_append(buffer, node->data.integer.content);
            break;
        case NODE_TYPE_OBJECT: {
            break;
        }
        case NODE_TYPE_PROPERTY: {
            Str type = property_return_type(code_gen, node);
            if (node->data.property.property_value->type == NODE_TYPE_OBJECT) {
                for (i64 i = 0;
                     i <
                     array_length(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    AstNode* n = node->data.property.property_value->data
                                        .object.property_list[i];
                    generate_node(code_gen, n, buffer, prefix, j + 1);
                }
                str_buffer_append_printf(
                    buffer, str_new((u8*)"" str_fmt " {\n"), str_args(type)
                );
                for (i64 i = 0;
                     i <
                     array_length(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    AstNode* n = node->data.property.property_value->data
                                        .object.property_list[i];
                    Str child_type = property_return_type(code_gen, n);
                    str_buffer_append_printf(
                        buffer,
                        str_new((u8*)""
                                "    " str_fmt " (*" str_fmt ")("),
                        str_args(child_type),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        )
                    );
                    str_buffer_append(buffer, str_new((u8*)");\n"));
                }

                str_buffer_append(buffer, str_new((u8*)"};\n"));

                str_buffer_append_printf(
                    buffer, str_new((u8*)"" str_fmt " " str_fmt "("), str_args(type),
                    str_args(
                        node->data.property.identifier->data.identifier.content
                    )
                );
                for (i64 i = 0;
                     i <
                     array_length(node->data.property.property_value
                                               ->data.object.free_list);
                     ++i) {
                    if (i != 0) {
                        str_buffer_append(buffer, str_new((u8*)", "));
                    }
                    str_buffer_append_printf(
                        buffer, str_new((u8*)"int param%ld"), i
                    );
                    // AstNode* n =
                    // node->data.property.property_value->data.object.free_list[i];
                }
                str_buffer_append(buffer, str_new((u8*)") {\n"));
                str_buffer_append_printf(
                    buffer, str_new((u8*)"    return (" str_fmt "){\n"),
                    str_args(type)
                );
                for (i64 i = 0;
                     i <
                     array_length(node->data.property.property_value
                                               ->data.object.property_list);
                     ++i) {
                    AstNode* n = node->data.property.property_value->data
                                        .object.property_list[i];
                    str_buffer_append_printf(
                        buffer,
                        str_new((u8*)"        ." str_fmt " = " str_fmt ",\n"),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        ),
                        str_args(
                            n->data.property.identifier->data.identifier.content
                        )
                    );
                }
                str_buffer_append_printf(
                    buffer, str_new((u8*)"    };\n"
                                    "}\n")
                );
            } else {
                str_buffer_append_printf(
                    buffer,
                    str_new((u8*)str_fmt " " str_fmt "(void) {\n    return "),
                    str_args(type),
                    str_args(
                        node->data.property.identifier->data.identifier.content
                    )
                );

                generate_node(
                    code_gen, node->data.property.property_value, buffer,
                    prefix, j + 1
                );

                str_buffer_append_printf(buffer, str_new((u8*)";\n}\n"));
            }
            break;
        }
        case NODE_TYPE_IDENTIFIER:
            break;
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS: {
            str_buffer_append_char(buffer, '(');
            for (i64 i = 0;
                 i < array_length(
                         node->data.free_object_copy_params.parameter_list
                     );
                 ++i) {
                if (i != 0) {
                    str_buffer_append(buffer, str_new((u8*)", "));
                }
                AstNode* n =
                    node->data.free_object_copy_params.parameter_list[i];
                generate_node(code_gen, n, buffer, prefix, j + 1);
            }
            str_buffer_append_char(buffer, ')');
            break;
        }
        case NODE_TYPE_OBJECT_COPY: {
            Str identifier =
                node->data.object_copy.identifier->data.identifier.content;
            if (str_equal(identifier, str_new((u8*)"print"))) {
                assert(
                    array_length(
                        node->data.object_copy.free_object_copies
                    ) == 1
                );
                assert(node->data.object_copy.object_copy == nullptr);
                AstNode* copies =
                    node->data.object_copy.free_object_copies[0];
                assert(
                    array_length(
                        copies->data.free_object_copy_params.parameter_list
                    ) == 1
                );
                str_buffer_append_printf(
                    buffer, str_new((u8*)"printf(\"%%d\", " str_fmt ")"),
                    str_args(copies->data.free_object_copy_params
                                 .parameter_list[0]
                                 ->data.integer.content)
                );
            } else {
                str_buffer_append_printf(
                    buffer, str_new((u8*)str_fmt),
                    str_args(node->data.object_copy.identifier->data.identifier
                                 .content)
                );
                if (array_length(
                        node->data.object_copy.free_object_copies
                    ) != 0) {
                    for (i64 i = 0;
                         i < array_length(
                                 node->data.object_copy.free_object_copies
                             );
                         ++i) {
                        AstNode* n =
                            node->data.object_copy.free_object_copies[i];
                        generate_node(code_gen, n, buffer, prefix, j + 1);
                    }
                } else {
                    str_buffer_append(buffer, str_new((u8*)"()"));
                }
                if (node->data.object_copy.object_copy != nullptr) {
                    str_buffer_append(buffer, str_new((u8*)"."));
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
    CodeGen code_gen[static 1], StrBuffer buffer[static 1],
    AstNode root[static 1]
) {
    write_program_start(buffer, root);

    generate_node(code_gen, root, buffer, str_new((u8*)"_root"), 0);

    write_program_end(code_gen, buffer);
}
