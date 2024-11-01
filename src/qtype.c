#include "allocator.h"
#include "ast.h"
#include "generator.h"
#include "qbe.h"
#include "util.h"
#include <stdio.h>

static u32 id = 0;

static struct qbe_type*
aggregate_lookup(
    struct arena* arena, struct qbe_program* program, struct ast_type* type
) {
    for (auto definition = program->definitions; definition != nullptr;
         definition      = definition->next) {
        if (definition->definition_type == QBE_DEFINITION_TYPE_TYPE
            && definition->type.base == type) {
            return &definition->type;
        }
    }

    struct qbe_definition* definition
        = arena_allocate(arena, sizeof(struct qbe_definition));
    definition->definition_type = QBE_DEFINITION_TYPE_TYPE;
    id += 1;
    definition->name       = generate_name(arena, id, from_cstr("type.%d"));
    definition->type.stype = QBE_STYPE_AGGREGATE;
    definition->type.base  = type;
    definition->type.name  = definition->name;

    auto field = &definition->type.fields;
    switch (type->extra_type) {
        case AST_EXTRA_STYPE_INTEGER:
            field->type  = &qbe_long;
            field->count = 1;
            break;
        case AST_EXTRA_STYPE_STRING:
            field->type  = &qbe_long;
            field->count = 3;
            break;
        case AST_EXTRA_STYPE_NONE: {
            switch (type->type) {
                case AST_STYPE_ANY:
                case AST_STYPE_COPY:
                case AST_STYPE_ALIAS:
                    break;
                case AST_STYPE_OBJECT:
                    for (auto object_type = type->object_types;
                         object_type != nullptr;
                         object_type = object_type->next) {
                        if (object_type->type->size != 0) {
                            field->type = qtype_lookup(
                                arena, program, object_type->type
                            );
                            field->count = 1;
                        }

                        if (object_type->next != nullptr
                            && object_type->next->type->size != 0) {
                            field->next = arena_allocate(
                                arena, sizeof(struct qbe_field)
                            );
                            field = field->next;
                        }
                    }
                    break;
                case AST_STYPE_UNION:
                    definition->type.stype = QBE_STYPE_UNION;
                    for (auto union_type = type->union_type;
                         union_type != nullptr;
                         union_type = union_type->next) {
                        if (union_type->type->size != 0) {
                            field->type = qtype_lookup(
                                arena, program, union_type->type
                            );
                            field->count = 1;
                        }

                        if (union_type->next != nullptr
                            && union_type->next->type->size != 0) {
                            field->next = arena_allocate(
                                arena, sizeof(struct qbe_field)
                            );
                            field = field->next;
                        }
                    }
                    break;
            }
            break;
        }
    }

    qbe_append_definition(program, definition);
    return &definition->type;
}

struct qbe_type*
qtype_lookup(
    struct arena* arena, struct qbe_program* program, struct ast_type* type
) {
    if (type == nullptr) {
        return nullptr;
    }
    switch (type->type) {
        case AST_STYPE_OBJECT:
        case AST_STYPE_UNION:
            return aggregate_lookup(arena, program, type);
        case AST_STYPE_ALIAS:
            return qtype_lookup(arena, program, type->alias_type);
        case AST_STYPE_ANY:
        case AST_STYPE_COPY:
            printf("%d\n", type->type);
            return nullptr;
    }
    vix_unreachable();
}
