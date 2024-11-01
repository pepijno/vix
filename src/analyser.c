#include "analyser.h"

#include "allocator.h"
#include "ast.h"
#include "parser.h"
#include "scope.h"
#include "util.h"

#include <assert.h>

static void
update_property_type(
    struct arena* arena, struct ast_property* property,
    struct ast_object* new_type
) {
    if (property->object->type.type == AST_STYPE_ANY) {
        property->object->type.type       = AST_STYPE_ALIAS;
        property->object->type.alias_type = &new_type->type;
        return;
    } else if (property->object->type.type != AST_STYPE_UNION) {
        struct ast_union_type* union_type
            = arena_allocate(arena, sizeof(struct ast_union_type));
        union_type->type       = arena_allocate(arena, sizeof(struct ast_type));
        union_type->type->type = AST_STYPE_ALIAS;
        if (property->object->type.type == AST_STYPE_ALIAS) {
            union_type->type->alias_type = property->object->type.alias_type;
        } else {
            union_type->type->alias_type = &property->object->type;
        }
        property->object->type.type       = AST_STYPE_UNION;
        property->object->type.union_type = union_type;
    }

    struct ast_union_type* union_type
        = arena_allocate(arena, sizeof(struct ast_union_type));
    union_type->type       = arena_allocate(arena, sizeof(struct ast_type));
    union_type->type->type = AST_STYPE_ALIAS;
    if (property->object->type.type == AST_STYPE_ALIAS) {
        union_type->type->alias_type = new_type->type.alias_type;
    } else {
        union_type->type->alias_type = &new_type->type;
    }
    union_type->next                  = property->object->type.union_type;
    property->object->type.union_type = union_type;
}

static void
update_copy_types(struct context* context, struct ast_object* object) {
    struct scope* scope = scope_push(context->arena, &context->scope);

    struct ast_property* property = object->properties;

    while (property != nullptr) {
        scope_insert_property(context->arena, scope, property);
        property = property->next;
    }

    if (object->type.type == AST_STYPE_COPY) {
        struct string name                = object->object_copy->name;
        struct scope_object* scope_object = scope_lookup_name(scope, name);
        if (scope_object != nullptr) {
            u32 current_id = scope_object->property->object->id;
            struct ast_property* current_property = scope_object->property;
            object->type.type                     = AST_STYPE_ALIAS;
            object->type.alias_type = &scope_object->property->object->type;

            struct ast_property* scope_object_property
                = current_property->object->properties;
            struct ast_free_property_assign* free_prop_assign
                = object->object_copy->free_properties;
            while (free_prop_assign != nullptr) {
                assert(scope_object_property->type == AST_PROPERTY_TYPE_FREE);

                update_property_type(
                    context->arena, scope_object_property,
                    free_prop_assign->value
                );

                free_prop_assign      = free_prop_assign->next;
                scope_object_property = scope_object_property->next;
            }

            struct ast_object_copy* object_copy = object->object_copy->next;
            while (object_copy != nullptr) {
                object_copy = object_copy->next;
            }
        } else {
            vix_unreachable();
        }
    }

    property = object->properties;
    while (property != nullptr) {
        // scope_insert_object(context->arena, scope, property->object);
        if (property->type == AST_PROPERTY_TYPE_NON_FREE) {
            update_copy_types(context, property->object);
        }
        property = property->next;
    }

    scope_pop(&context->scope);
}

static u32
update_type_size(struct ast_type* type) {
    if (type->size != 0) {
        return type->size;
    }

    u32 size = 0;
    switch (type->extra_type) {
        case AST_EXTRA_STYPE_NONE:
            break;
        case AST_EXTRA_STYPE_INTEGER:
            size += 8;
            break;
        case AST_EXTRA_STYPE_STRING:
            size += 24;
            break;
    }

    switch (type->type) {
        case AST_STYPE_ANY:
        case AST_STYPE_COPY:
        case AST_STYPE_OBJECT:
            for (auto object_type = type->object_types; object_type != nullptr;
                 object_type      = object_type->next) {
                size += update_type_size(object_type->type);
            }
            break;
        case AST_STYPE_UNION:
            u32 max = 0;
            for (auto union_type = type->union_type; union_type != nullptr;
                 union_type      = union_type->next) {
                u32 s = update_type_size(union_type->type);
                if (s > max) {
                    max = s;
                }
            }
            size += max;
            break;
        case AST_STYPE_ALIAS: {
            size += update_type_size(type->alias_type);
            break;
        }
    }

    type->size = size;
    return size;
}

static void
update_type_sizes() {
    for (usize i = 0; i < BUCKET_SIZE; ++i) {
        for (auto entry = buckets[i]; entry != nullptr; entry = entry->next) {
            if (entry->type == PARSER_ENTRY_TYPE_PROPERTY) {
                continue;
            }
            update_type_size(&entry->object->type);
        }
    }
}

void
analyse(struct context* context, struct ast_object* root) {
    scope_push(context->arena, &context->scope);
    update_copy_types(context, root);
    update_type_sizes();
    scope_pop(&context->scope);
}
