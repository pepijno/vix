#include "analyser.h"

#include "allocator.h"
#include "ast.h"
#include "util.h"

#include <stdio.h>

// struct function_type function_types[1024];
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
// static struct function_type*
// update_types(struct arena* arena, struct ast_object object[static 1]) {
//     struct function_type* type = &function_types[object->id];
//
//     type->param_types
//         = arena_allocate(arena, sizeof(struct function_param_type));
//     type->id                            = object->id;
//     struct function_param_type** next   = &type->param_types;
//     struct ast_free_property* free_prop = object->free_properties;
//     while (free_prop != nullptr) {
//         (*next)->name = string_duplicate(arena, free_prop->name);
//         (*next)->next
//             = arena_allocate(arena, sizeof(struct function_param_type));
//         (*next)->function_type     = &function_types[free_prop->id];
//         (*next)->function_type->id = free_prop->id;
//         (*next)->function_type->return_type.return_type = RETURN_ANY;
//         next                                            = &(*next)->next;
//         free_prop                                       = free_prop->next;
//     }
//     *next = nullptr;
//
//     switch (object->type) {
//         case OBJECT_TYPE_NONE:
//             break;
//         case OBJECT_TYPE_OBJECT_COPY: {
//             type->return_type.return_type = RETURN_COPY;
//             struct ast_object_copy* copy  = object->object_copy;
//             struct nearest_object nearest_object
//                 = find_nearest_object(object, copy->name);
//             i32 id                      = nearest_object.id;
//             type->return_type.copy_type = &function_types[id];
//
//             if (nearest_object.object != nullptr
//                 && function_types[id].id == -1) {
//                 update_types(arena, nearest_object.object);
//             } else if (nearest_object.free_prop != nullptr
//                        && function_types[id].id == -1) {
//                 function_types[id].id                      = free_prop->id;
//                 function_types[id].return_type.return_type = RETURN_ANY;
//             }
//
//             struct ast_free_property_assign* free_assign
//                 = copy->free_properties;
//             struct function_param_type* param_type
//                 = type->return_type.copy_type->param_types;
//             while (free_assign != nullptr) {
//                 if (function_types[free_assign->value->id].id == -1) {
//                     update_types(arena, free_assign->value);
//                 }
//                 if (param_type->function_type->return_type.return_type
//                     == RETURN_ANY) {
//                     param_type->function_type->return_type.return_type
//                         = RETURN_UNION;
//                     param_type->function_type->return_type.function_union
//                         = arena_allocate(arena, sizeof(struct function_union));
//                     param_type->function_type->return_type.function_union->type
//                         = &function_types[free_assign->value->id];
//                     param_type->function_type->return_type.function_union->next
//                         = nullptr;
//                 } else if (param_type->function_type->return_type.return_type
//                            == RETURN_UNION) {
//                     struct function_union* last
//                         = param_type->function_type->return_type.function_union;
//                     while (last != nullptr) {
//                         if (last->next != nullptr) {
//                             last = last->next;
//                         } else {
//                             break;
//                         }
//                     }
//                     last->next
//                         = arena_allocate(arena, sizeof(struct function_union));
//                     last->next->type = &function_types[free_assign->value->id];
//                     last->next->next = nullptr;
//                 } else {
//                     vix_unreachable();
//                 }
//
//                 param_type  = param_type->next;
//                 free_assign = free_assign->next;
//             }
//
//             break;
//         }
//         case OBJECT_TYPE_PROPERTIES: {
//             type->return_type.return_type = RETURN_OBJECT;
//             type->return_type.properties
//                 = arena_allocate(arena, sizeof(struct return_object_property));
//             struct return_object_property** next_prop
//                 = &type->return_type.properties;
//
//             struct ast_property* prop = object->properties;
//             while (prop != nullptr) {
//                 (*next_prop)->name = string_duplicate(arena, prop->name);
//
//                 (*next_prop)->function_type = update_types(arena, prop->object);
//
//                 (*next_prop)->next = arena_allocate(
//                     arena, sizeof(struct return_object_property)
//                 );
//                 next_prop = &(*next_prop)->next;
//                 prop      = prop->next;
//             }
//
//             *next_prop = nullptr;
//
//             break;
//         }
//         case OBJECT_TYPE_INTEGER: {
//             type->return_type.return_type = RETURN_INTEGER;
//             break;
//         }
//         case OBJECT_TYPE_STRING: {
//             type->return_type.return_type = RETURN_STRING;
//             break;
//         }
//     }
//
//     return type;
// }
//
// struct string
// type_to_string(enum return_type type) {
//     switch (type) {
//         case RETURN_INTEGER:
//             return from_cstr("INTEGER");
//         case RETURN_STRING:
//             return from_cstr("STRING");
//         case RETURN_OBJECT:
//             return from_cstr("OBJECT");
//         case RETURN_COPY:
//             return from_cstr("COPY");
//         case RETURN_UNION:
//             return from_cstr("UNION");
//         case RETURN_ANY:
//         default:
//             return from_cstr("ANY");
//     }
// }
//
// void
// print_types(struct function_type* type, u8 indent) {
//     if (type->param_types != nullptr) {
//         printf("%*sfrees:\n", indent, "");
//         struct function_param_type* param_type = type->param_types;
//         while (param_type != nullptr) {
//             printf(
//                 "%*s  %d " STR_FMT ":\n", indent, "",
//                 param_type->function_type->id, STR_ARG(param_type->name)
//             );
//             print_types(param_type->function_type, indent + 4);
//             param_type = param_type->next;
//         }
//     }
//
//     printf(
//         "%*stype " STR_FMT "\n", indent, "",
//         STR_ARG(type_to_string(type->return_type.return_type))
//     );
//     if (type->return_type.return_type == RETURN_OBJECT) {
//         struct return_object_property* prop = type->return_type.properties;
//         while (prop != nullptr) {
//             printf(
//                 "%*s  %d " STR_FMT ":\n", indent, "", prop->function_type->id,
//                 STR_ARG(prop->name)
//             );
//             print_types(prop->function_type, indent + 4);
//             prop = prop->next;
//         }
//     } else if (type->return_type.return_type == RETURN_COPY) {
//         printf("%*scopy %d:\n", indent, "", type->return_type.copy_type->id);
//         print_types(type->return_type.copy_type, indent + 2);
//     } else if (type->return_type.return_type == RETURN_UNION) {
//         printf("%*sunion:\n", indent, "");
//         struct function_union* function_union
//             = type->return_type.function_union;
//         while (function_union != nullptr) {
//             print_types(function_union->type, indent + 2);
//             function_union = function_union->next;
//         }
//     }
// }

struct function_type*
analyse(struct arena* arena, struct ast_object* root) {
    // for (usize i = 0; i < 1024; ++i) {
    //     function_types[i].id = -1;
    // }
    //
    // struct function_type* root_type = update_types(arena, root);
    // print_types(root_type, 0);
    // return root_type;
}
