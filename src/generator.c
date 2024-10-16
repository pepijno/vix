#include "generator.h"

#include "analyser.h"
#include "parser.h"
#include "qbe.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

u32 id = 0;

static char*
generate_name(char const* format) {
    i32 n        = snprintf(nullptr, 0, format, id);
    char* string = calloc(n + 1, sizeof(char));
    snprintf(string, n + 1, format, id);
    id += 1;
    return string;
}

static void
generate_data_item(
    struct qbe_program program[static 1], struct ast_object object[static 1],
    struct qbe_data_item item[static 1]
) {
    struct qbe_definition* definition;

    switch (object->type) {
        case OBJECT_TYPE_INTEGER: {
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(object->integer);
            break;
        }
        case OBJECT_TYPE_STRING: {
            definition       = calloc(1, sizeof(struct qbe_definition));
            definition->name = generate_name("string_data.%d");
            definition->definition_type      = QBE_DEFINITION_TYPE_DATA;
            definition->data.align           = ALIGN_UNDEFINED;
            definition->data.items.data_type = QBE_DATA_TYPE_STRINGS;
            definition->data.items.string
                = calloc(strlen(object->string) + 1, sizeof(char));
            definition->data.items.size = strlen(object->string) + 1;
            memcpy(
                definition->data.items.string, object->string,
                strlen(object->string) + 1
            );

            item->data_type = QBE_DATA_TYPE_VALUE;
            if (strlen(object->string) != 0) {
                qbe_append_definition(program, definition);
                item->value.value_type = QBE_VALUE_TYPE_GLOBAL;
                item->value.type       = &qbe_long;
                item->value.name       = strdup(definition->name);
            } else {
                free(definition);
                item->value = const_u64(0);
            }

            item->next      = calloc(1, sizeof(struct qbe_data_item));
            item            = item->next;
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(strlen(object->string) + 1);

            item->next      = calloc(1, sizeof(struct qbe_data_item));
            item            = item->next;
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(strlen(object->string) + 1);
            break;
        }
        case OBJECT_TYPE_NONE:
        case OBJECT_TYPE_OBJECT_COPY:
        case OBJECT_TYPE_PROPERTIES:
            break;
    }
}

static void
generate_string_literal(
    struct qbe_program program[static 1], struct ast_object object[static 1]
) {
    assert(object->type == OBJECT_TYPE_STRING);
    struct qbe_definition* definition
        = calloc(1, sizeof(struct qbe_definition));
    definition->definition_type = QBE_DEFINITION_TYPE_DATA;
    definition->data.align      = ALIGN_UNDEFINED;
    definition->name            = generate_name("string_literal.%d");

    generate_data_item(program, object, &definition->data.items);
    qbe_append_definition(program, definition);
}

static void
generate_integer_literal(
    struct qbe_program program[static 1], struct ast_object object[static 1]
) {
    assert(object->type == OBJECT_TYPE_INTEGER);
    struct qbe_definition* definition
        = calloc(1, sizeof(struct qbe_definition));
    definition->definition_type = QBE_DEFINITION_TYPE_DATA;
    definition->name            = generate_name("integer_literal.%d");

    generate_data_item(program, object, &definition->data.items);
    qbe_append_definition(program, definition);
}

size sizes[1024];

struct nearest_object {
    i32 id;
    struct ast_free_property* free_prop;
    struct ast_object* object;
};

static struct nearest_object
find_nearest_object(struct ast_object object[static 1], char* name) {
    struct ast_free_property* free_prop = object->free_properties;
    while (free_prop != nullptr) {
        if (strcmp(free_prop->name, name) == 0) {
            return (struct nearest_object){
                .id        = free_prop->id,
                .free_prop = free_prop,
                .object    = nullptr,
            };
        }
        free_prop = free_prop->next;
    }
    if (object->type == OBJECT_TYPE_PROPERTIES) {
        struct ast_property* prop = object->properties;
        while (prop != nullptr) {
            if (strcmp(prop->name, name) == 0) {
                return (struct nearest_object){
                    .id        = prop->object->id,
                    .free_prop = nullptr,
                    .object    = prop->object,
                };
            }
            prop = prop->next;
        }
    }
    if (object->id == 0) {
        vix_panic("bla");
    }
    return find_nearest_object(object->parent, name);
}

static bool
all_free_props_any(struct function_type function_type[static 1]) {
    struct function_param_type* param_type = function_type->param_types;
    bool allAny                            = true;
    while (param_type != nullptr) {
        if (param_type->function_type->return_type.return_type != RETURN_ANY) {
            allAny = false;
            break;
        }
        param_type = param_type->next;
    }
    return allAny;
}

static void
generate_constants(
    struct qbe_program program[static 1], struct ast_object object[static 1]
) {
    switch (object->type) {
        case OBJECT_TYPE_NONE:
            break;
        case OBJECT_TYPE_OBJECT_COPY: {
            struct ast_free_property_assign* prop_assign
                = object->object_copy->free_properties;
            while (prop_assign != nullptr) {
                generate_constants(program, prop_assign->value);
                prop_assign = prop_assign->next;
            }
            break;
        }
        case OBJECT_TYPE_PROPERTIES: {
            struct ast_property* prop = object->properties;
            while (prop != nullptr) {
                generate_constants(program, prop->object);
                prop = prop->next;
            }
            break;
        }
        case OBJECT_TYPE_INTEGER: {
            generate_integer_literal(program, object);
            break;
        }
        case OBJECT_TYPE_STRING: {
            generate_string_literal(program, object);
            break;
        }
    }
}

void
generate(
    struct qbe_program program[static 1], struct ast_object root[static 1],
    struct function_type root_type[static 1]
) {
    generate_constants(program, root);
}
