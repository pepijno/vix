#include "generator.h"

#include "allocator.h"
#include "analyser.h"
#include "ast.h"
#include "parser.h"
#include "qbe.h"
#include "util.h"

#include <assert.h>

static void
generate_data_item(
    struct arena* arena, struct qbe_program* program, struct ast_object* object,
    struct qbe_data_item* item
) {
    struct qbe_definition* definition;

    switch (object->type.extra_type) {
        case AST_EXTRA_STYPE_INTEGER: {
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(object->integer);
            break;
        }
        case AST_EXTRA_STYPE_STRING: {
            definition = arena_allocate(arena, sizeof(struct qbe_definition));
            definition->name
                = generate_name(arena, object->id, from_cstr("string_data.%d"));
            definition->definition_type      = QBE_DEFINITION_TYPE_DATA;
            definition->data.align           = ALIGN_UNDEFINED;
            definition->data.items.data_type = QBE_DATA_TYPE_STRINGS;
            definition->data.items.string
                = string_duplicate(arena, object->string);

            item->data_type = QBE_DATA_TYPE_VALUE;
            if (object->string.length != 0) {
                qbe_append_definition(program, definition);
                item->value.value_type = QBE_VALUE_TYPE_GLOBAL;
                item->value.type       = &qbe_long;
                item->value.name = string_duplicate(arena, definition->name);
            } else {
                item->value = const_u64(0);
            }

            item->next = arena_allocate(arena, sizeof(struct qbe_data_item));
            item       = item->next;
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(object->string.length + 1);

            item->next = arena_allocate(arena, sizeof(struct qbe_data_item));
            item       = item->next;
            item->data_type = QBE_DATA_TYPE_VALUE;
            item->value     = const_u64(object->string.length + 1);
            break;
        }
        case AST_EXTRA_STYPE_NONE:
            break;
    }
}

static void
generate_string_literal(
    struct arena* arena, struct qbe_program* program, struct ast_object* object
) {
    assert(object->type.extra_type == AST_EXTRA_STYPE_STRING);
    struct qbe_definition* definition
        = arena_allocate(arena, sizeof(struct qbe_definition));
    definition->definition_type = QBE_DEFINITION_TYPE_DATA;
    definition->data.align      = ALIGN_UNDEFINED;
    definition->name
        = generate_name(arena, object->id, from_cstr("string_literal.%d"));

    generate_data_item(arena, program, object, &definition->data.items);
    qbe_append_definition(program, definition);
}

static void
generate_integer_literal(
    struct arena* arena, struct qbe_program* program, struct ast_object* object
) {
    assert(object->type.extra_type == AST_EXTRA_STYPE_INTEGER);
    struct qbe_definition* definition
        = arena_allocate(arena, sizeof(struct qbe_definition));
    definition->definition_type = QBE_DEFINITION_TYPE_DATA;
    definition->name
        = generate_name(arena, object->id, from_cstr("integer_literal.%d"));

    generate_data_item(arena, program, object, &definition->data.items);
    qbe_append_definition(program, definition);
}

static void
generate_constants(struct arena* arena, struct qbe_program* program) {
    for (usize i = 0; i < BUCKET_SIZE; ++i) {
        auto entry = buckets[i];
        while (entry != nullptr) {
            if (entry->type == PARSER_ENTRY_TYPE_OBJECT) {
                switch (entry->object->type.extra_type) {
                    case AST_EXTRA_STYPE_NONE:
                        break;
                    case AST_EXTRA_STYPE_INTEGER:
                        generate_integer_literal(arena, program, entry->object);
                        break;
                    case AST_EXTRA_STYPE_STRING:
                        generate_string_literal(arena, program, entry->object);
                        break;
                }
            }
            entry = entry->next;
        }
    }
}

void
generate(
    struct arena* arena, struct qbe_program* program, struct ast_object* root
) {
    generate_constants(arena, program);
    qtype_lookup(arena, program, &root->type);
}
