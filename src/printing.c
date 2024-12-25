#include "ast.h"
#include "defs.h"
#include "instructions.h"
#include "types.h"
#include "vector.h"

#include <stdio.h>

static void
print_indent(usize indent) {
    for (usize i = 0; i < indent; ++i) {
        printf("  ");
    }
}

void print_element(struct _ast_element* element, usize indent);

static void
print_property(struct _ast_property* property, usize indent) {
    if (property == nullptr) {
        return;
    }
    print_indent(indent);
    fprintf(
        stdout, "PROPERTY %lu: " STR_FMT "\n", property->id,
        STR_ARG(property->name)
    );
    print_element(property->value, indent + 1);
}

void
print_element(struct _ast_element* element, usize indent) {
    if (element == nullptr) {
        return;
    }

    switch (element->type) {
        case AST_ELEMENT_TYPE_ID:
            print_indent(indent);
            printf("ID: " STR_FMT "\n", STR_ARG(element->id.id));
            break;
        case AST_ELEMENT_TYPE_STRING:
            print_indent(indent);
            printf("STRING: " STR_FMT "\n", STR_ARG(element->string.value));
            break;
        case AST_ELEMENT_TYPE_INTEGER:
            print_indent(indent);
            printf("INT: %ld\n", element->integer.value);
            break;
        case AST_ELEMENT_TYPE_PROPERTIES:
            print_indent(indent);
            printf("PROPERTIES: \n");
            vector_foreach(element->properties, prop) {
                print_property(prop, indent + 1);
            }
            break;
    }
}

void
_print_type(struct type_context* context, struct type* type, usize indent) {
    switch (type->type) {
        case TYPE_TYPE_VAR:
            auto found
                = hashmap_string_type_find(&context->types, type->var.name);
            if (found != nullptr) {
                _print_type(context, found, indent);
            } else {
                print_indent(indent);
                printf("VAR " STR_FMT "\n", STR_ARG(type->var.name));
            }
            break;
        case TYPE_TYPE_BASE:
            print_indent(indent);
            printf(STR_FMT "\n", STR_ARG(type->base.name));
            break;
        case TYPE_TYPE_ARROW:
            print_indent(indent);
            _print_type(context, type->arrow.left, indent);
            printf(" -> (");
            _print_type(context, type->arrow.right, indent);
            printf(")\n");
            break;
        case TYPE_TYPE_PROPERTIES:
            print_indent(indent);
            printf("{\n");
            vector_foreach(type->properties, prop) {
                print_indent(indent + 1);
                printf(STR_FMT ":\n", STR_ARG(prop->name));
                _print_type(context, prop->type, indent + 2);
            }
            print_indent(indent);
            printf("}\n");
            break;
    }
}

void
print_instruction(struct instruction instruction, usize indent) {
    print_indent(indent);
    switch (instruction.type) {
        case INSTRUCTION_TYPE_PUSH_INT:
            printf("push_int(%ld)\n", instruction.push_int.value);
            break;
        case INSTRUCTION_TYPE_PUSH_STR:
            printf(
                "push_str(" STR_FMT ")\n", STR_ARG(instruction.push_str.value)
            );
            break;
        case INSTRUCTION_TYPE_PUSH_GLOBAL:
            printf(
                "push_global(" STR_FMT ")\n",
                STR_ARG(instruction.push_global.name)
            );
            break;
        case INSTRUCTION_TYPE_PUSH:
            printf("push(%ld)\n", instruction.push.offset);
            break;
        case INSTRUCTION_TYPE_PACK:
            printf("pack(%ld)\n", instruction.pack.size);
            break;
        case INSTRUCTION_TYPE_SPLIT:
            printf("split(%ld)\n", instruction.split.size);
            break;
    }
}
