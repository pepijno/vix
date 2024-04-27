#include "analyser.h"

#include "array.h"
#include "error_message.h"
#include "parser.h"
#include "util.h"

static ErrorMessage* add_node_error(
    Analyzer code_gen[static 1], AstNode node[static 1], Str message
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
    Analyzer code_gen[static 1], ErrorMessage parent_message[static 1],
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
        if (str_equal(scope->properties[i]->identifier.content, identifier)) {
            return scope->properties[i];
        }
    }
    return nullptr;
}

static void analyse_node(
    Analyzer code_gen[static 1], Scope scope[static 1], AstNode node[static 1]
);

static void analyse_property(
    Analyzer code_gen[static 1], Scope scope[static 1],
    AstNode property[static 1]
);

static void analyse_identifier(
    Analyzer code_gen[static 1], Scope scope[static 1],
    AstNode identifier[static 1]
) {
    Str identifier_content       = identifier->identifier.content;
    AstNode* existing_identifier = find_identifier(scope, identifier_content);
    if (existing_identifier) {
        StrBuffer buffer = str_buffer_new(code_gen->allocator, 0);
        str_buffer_printf(
            &buffer, str_new("redefinition of '%s'"), identifier_content
        );
        ErrorMessage* error_message =
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
    Analyzer code_gen[static 1], Scope scope[static 1], AstNode root[static 1]
) {
    for (i64 i = 0; i < array_length(root->root.list); ++i) {
        analyse_node(code_gen, scope, root->root.list[i]);
    }
}

static void analyse_object(
    Analyzer code_gen[static 1], Scope scope[static 1], AstNode object[static 1]
) {
    Scope new_scope = {
        .parent      = scope,
        .source_node = object,
    };
    new_scope.properties = array(AstNode*, code_gen->allocator);

    for (i64 i = 0; i < array_length(object->object.free_list); ++i) {
        analyse_identifier(code_gen, &new_scope, object->object.free_list[i]);
    }
    for (i64 i = 0; i < array_length(object->object.property_list); ++i) {
        AstNode* node = object->object.property_list[i];
        if (node->type == NODE_TYPE_PROPERTY) {
            analyse_property(
                code_gen, &new_scope, object->object.property_list[i]
            );
        } else if (node->type == NODE_TYPE_DECORATOR) {
            // TODO
        } else {
            vix_unreachable();
        }
    }
}

static void analyse_property(
    Analyzer code_gen[static 1], Scope scope[static 1],
    AstNode property[static 1]
) {
    assert(property->property.identifier->identifier.content.length != 0);
    analyse_identifier(code_gen, scope, property->property.identifier);
    analyse_node(code_gen, scope, property->property.property_value);
}

static void analyse_node(
    Analyzer code_gen[static 1], Scope scope[static 1], AstNode node[static 1]
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

static void report_errors_and_maybe_exit(Analyzer code_gen[static 1]) {
    if (array_length(code_gen->errors) > 0) {
        for (i64 i = 0; i < array_length(code_gen->errors); ++i) {
            ErrorMessage* error_message = code_gen->errors[i];
            print_error_message(error_message, code_gen->error_color);
        }
        exit(1);
    }
}

void analyse(Analyzer code_gen[static 1], AstNode root[static 1]) {
    analyse_node(code_gen, &code_gen->root_scope, root);

    report_errors_and_maybe_exit(code_gen);
}

i64 global_id = 0;

static void write_program_start(
    StrBuffer buffer[static 1], AstNode root[static 1]
) {
    str_buffer_reset(buffer);
    str_buffer_append(buffer, str_new("#include <stdio.h>\n\n"));
}
