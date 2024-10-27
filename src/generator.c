#include "generator.h"

#include "allocator.h"
#include "analyser.h"
#include "ast.h"
#include "qbe.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>

// u32 id = 0;
//
// static struct string
// generate_name(struct arena* arena, struct string format) {
//     i32 n        = snprintf(nullptr, 0, format.data, id);
//     char* buffer = arena_allocate(arena, n + 1);
//     snprintf(buffer, n + 1, format.data, id);
//     id += 1;
//     return (struct string){
//         .data = buffer,
//         .length = n,
//     };
// }
//
// static void
// generate_data_item(
//     struct arena* arena, struct qbe_program* program, struct ast_object* object,
//     struct qbe_data_item* item
// ) {
//     struct qbe_definition* definition;
//
//     switch (object->type) {
//         case OBJECT_TYPE_INTEGER: {
//             item->data_type = QBE_DATA_TYPE_VALUE;
//             item->value     = const_u64(object->integer);
//             break;
//         }
//         case OBJECT_TYPE_STRING: {
//             definition = arena_allocate(arena, sizeof(struct qbe_definition));
//             definition->name = generate_name(arena, from_cstr("string_data.%d"));
//             definition->definition_type      = QBE_DEFINITION_TYPE_DATA;
//             definition->data.align           = ALIGN_UNDEFINED;
//             definition->data.items.data_type = QBE_DATA_TYPE_STRINGS;
//             definition->data.items.string
//                 = string_duplicate(arena, object->string);
//
//             item->data_type = QBE_DATA_TYPE_VALUE;
//             if (object->string.length != 0) {
//                 qbe_append_definition(program, definition);
//                 item->value.value_type = QBE_VALUE_TYPE_GLOBAL;
//                 item->value.type       = &qbe_long;
//                 item->value.name = string_duplicate(arena, definition->name);
//             } else {
//                 item->value = const_u64(0);
//             }
//
//             item->next = arena_allocate(arena, sizeof(struct qbe_data_item));
//             item       = item->next;
//             item->data_type = QBE_DATA_TYPE_VALUE;
//             item->value     = const_u64(object->string.length + 1);
//
//             item->next = arena_allocate(arena, sizeof(struct qbe_data_item));
//             item       = item->next;
//             item->data_type = QBE_DATA_TYPE_VALUE;
//             item->value     = const_u64(object->string.length + 1);
//             break;
//         }
//         case OBJECT_TYPE_NONE:
//         case OBJECT_TYPE_OBJECT_COPY:
//         case OBJECT_TYPE_PROPERTIES:
//             break;
//     }
// }
//
// static void
// generate_string_literal(
//     struct arena* arena, struct qbe_program* program, struct ast_object* object
// ) {
//     assert(object->type == OBJECT_TYPE_STRING);
//     struct qbe_definition* definition
//         = arena_allocate(arena, sizeof(struct qbe_definition));
//     definition->definition_type = QBE_DEFINITION_TYPE_DATA;
//     definition->data.align      = ALIGN_UNDEFINED;
//     definition->name            = generate_name(arena, from_cstr("string_literal.%d"));
//
//     generate_data_item(arena, program, object, &definition->data.items);
//     qbe_append_definition(program, definition);
// }
//
// static void
// generate_integer_literal(
//     struct arena* arena, struct qbe_program* program, struct ast_object* object
// ) {
//     assert(object->type == OBJECT_TYPE_INTEGER);
//     struct qbe_definition* definition
//         = arena_allocate(arena, sizeof(struct qbe_definition));
//     definition->definition_type = QBE_DEFINITION_TYPE_DATA;
//     definition->name            = generate_name(arena, from_cstr("integer_literal.%d"));
//
//     generate_data_item(arena, program, object, &definition->data.items);
//     qbe_append_definition(program, definition);
// }
//
// usize sizes[1024];
//
// struct nearest_object {
//     i32 id;
//     struct ast_free_property* free_prop;
//     struct ast_object* object;
// };
//
// static struct nearest_object
// find_nearest_object(struct ast_object* object, struct string name) {
//     struct ast_free_property* free_prop = object->free_properties;
//     while (free_prop != nullptr) {
//         if (strings_equal(free_prop->name, name)) {
//             return (struct nearest_object){
//                 .id        = free_prop->id,
//                 .free_prop = free_prop,
//                 .object    = nullptr,
//             };
//         }
//         free_prop = free_prop->next;
//     }
//     if (object->type == OBJECT_TYPE_PROPERTIES) {
//         struct ast_property* prop = object->properties;
//         while (prop != nullptr) {
//             if (strings_equal(prop->name, name)) {
//                 return (struct nearest_object){
//                     .id        = prop->object->id,
//                     .free_prop = nullptr,
//                     .object    = prop->object,
//                 };
//             }
//             prop = prop->next;
//         }
//     }
//     if (object->id == 0) {
//         vix_unreachable();
//     }
//     return find_nearest_object(object->parent, name);
// }
//
// static bool
// all_free_props_any(struct function_type* function_type) {
//     struct function_param_type* param_type = function_type->param_types;
//     bool allAny                            = true;
//     while (param_type != nullptr) {
//         if (param_type->function_type->return_type.return_type != RETURN_ANY) {
//             allAny = false;
//             break;
//         }
//         param_type = param_type->next;
//     }
//     return allAny;
// }
//
// static void
// generate_constants(
//     struct arena* arena, struct qbe_program* program, struct ast_object* object
// ) {
//     switch (object->type) {
//         case OBJECT_TYPE_NONE:
//             break;
//         case OBJECT_TYPE_OBJECT_COPY: {
//             struct ast_free_property_assign* prop_assign
//                 = object->object_copy->free_properties;
//             while (prop_assign != nullptr) {
//                 generate_constants(arena, program, prop_assign->value);
//                 prop_assign = prop_assign->next;
//             }
//             break;
//         }
//         case OBJECT_TYPE_PROPERTIES: {
//             struct ast_property* prop = object->properties;
//             while (prop != nullptr) {
//                 generate_constants(arena, program, prop->object);
//                 prop = prop->next;
//             }
//             break;
//         }
//         case OBJECT_TYPE_INTEGER: {
//             generate_integer_literal(arena, program, object);
//             break;
//         }
//         case OBJECT_TYPE_STRING: {
//             generate_string_literal(arena, program, object);
//             break;
//         }
//     }
// }

void
generate(
    struct arena* arena, struct qbe_program* program, struct ast_object* root,
    struct function_type* root_type
) {
    // generate_constants(arena, program, root);
}
