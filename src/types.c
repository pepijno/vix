#include "types.h"

#include "allocator.h"
#include "graph.h"
#include "hash.h"
#include "parser.h"
#include "str.h"
#include "util.h"
#include "vector.h"

#include <assert.h>
#include <stdio.h>

VECTOR_IMPL(struct type_property*, type_property_ptr)
HASHMAP_IMPL(
    struct string, struct type*, string_type, string_hash, strings_equal
)
static u32
usize_hash(usize const u) {
    return fnv1a_u64(HASH_INIT, u);
}
static u32
usize_equals(usize const u1, usize const u2) {
    return u1 == u2;
}
HASHMAP_IMPL(usize, struct ast_property*, properties, usize_hash, usize_equals)

HASHMAP_IMPL(struct string, usize, string_id, string_hash, strings_equal)

static struct string
new_type_name(struct type_context context[static const 1]) {
    auto temp = context->last_id;

    usize count = 2;
    while (temp != 0) {
        count += 1;
        temp /= 26;
    }
    temp = context->last_id;

    auto name    = string_init_empty(context->allocator, count);
    usize i      = 2;
    name.data[0] = '\'';
    name.data[1] = '\'';
    while (temp != 0) {
        name.data[i] = 'a' + temp % 26;
        temp /= 26;
        i += 1;
    }
    context->last_id += 1;
    return name;
}

static struct type*
create_type_var(struct type_context context[static const 1]) {
    auto name = new_type_name(context);
    struct type* type
        = allocator_allocate(context->allocator, sizeof(struct type));
    type->type     = TYPE_TYPE_VAR;
    type->var.name = name;
    return type;
}

static struct type*
resolve(
    struct type_context context[static const 1], struct type type[static 1],
    struct type* type_var[static const 1]
) {
    *type_var = nullptr;
    while (type->type == TYPE_TYPE_VAR) {
        auto it = hashmap_string_type_find_or_insert_new(&context->types, type->var.name);

        if (it == nullptr) {
            *type_var = type;
            break;
        }

        type = it;
    }

    return type;
}

static void
bind(
    struct type_context context[static const 1], struct string const name,
    struct type type[static const 1]
) {
    if (type->type == TYPE_TYPE_VAR && strings_equal(type->var.name, name)) {
        return;
    }
    hashmap_string_type_add(&context->types, name, type);
}

static void
unify(
    struct type_context context[static const 1], struct type left[static 1],
    struct type right[static 1]
) {
    struct type* lvar;
    struct type* rvar;

    left  = resolve(context, left, &lvar);
    right = resolve(context, right, &rvar);

    if (lvar != nullptr) {
        bind(context, lvar->var.name, right);
        return;
    } else if (rvar != nullptr) {
        bind(context, rvar->var.name, left);
        return;
    } else if (left->type == TYPE_TYPE_ARROW
               && right->type == TYPE_TYPE_ARROW) {
        unify(context, left->arrow.left, right->arrow.left);
        unify(context, left->arrow.right, right->arrow.right);
        return;
    } else if (left->type == TYPE_TYPE_BASE && right->type == TYPE_TYPE_BASE) {
        if (strings_equal(left->base.name, right->base.name)) {
            return;
        }
    }

    vix_unreachable();
}

static struct type*
type_env_lookup(
    struct type_env type_env[static const 1], struct string const name
) {
    auto it = hashmap_string_type_find_or_insert_new(&type_env->names, name);
    if (it != nullptr) {
        return it;
    } else if (type_env->parent != nullptr) {
        return type_env_lookup(type_env->parent, name);
    } else {
        return nullptr;
    }
}

void
env_bind(
    struct type_env type_env[static const 1], struct string const name,
    struct type type[static const 1]
) {
    hashmap_string_type_add(&type_env->names, name, type);
}

static struct type_env*
new_scope(struct type_env type_env[static const 1]) {
    struct type_env* new_env
        = allocator_allocate(type_env->allocator, sizeof(struct type_env));
    new_env->allocator = type_env->allocator;
    new_env->parent    = type_env;
    new_env->names     = hashmap_string_type_new(type_env->allocator);
    return new_env;
}

void
typecheck_properties_first(
    struct type_context context[static const 1],
    struct type_env env[static const 1], struct string const name,
    struct ast_element element[static const 1]
) {
    assert(element->type == AST_ELEMENT_TYPE_PROPERTIES);
    struct type* type
        = allocator_allocate(context->allocator, sizeof(struct type));
    type->type       = TYPE_TYPE_PROPERTIES;
    type->properties = vector_type_property_ptr_new(context->allocator);

    vector_foreach(element->properties, prop) {
        struct type* prop_type  = create_type_var(context);
        struct type_property* a = allocator_allocate(
            context->allocator, sizeof(struct type_property)
        );
        a->type = prop_type;
        a->name = string_duplicate(context->allocator, prop->name);
        vector_type_property_ptr_add(&type->properties, a);
        prop->type = prop_type;
    }

    env_bind(env, name, type);
}

void
typecheck_properties_second(
    struct type_context context[static const 1],
    struct type_env env[static const 1],
    struct ast_element element[static const 1]
) {
    assert(element->type == AST_ELEMENT_TYPE_PROPERTIES);
    auto new_env = new_scope(env);

    vector_foreach(element->properties, prop) {
        auto typechecked_type = typecheck(context, env, prop->value);
        if (prop->value->type == AST_ELEMENT_TYPE_PROPERTIES) {
            typechecked_type = type_env_lookup(env, prop->name);
        }
        unify(context, prop->type, typechecked_type);
        env_bind(new_env, prop->name, prop->type);
    }
}

struct type*
typecheck(
    struct type_context context[static const 1],
    struct type_env env[static const 1],
    struct ast_element element[static const 1]
) {
    switch (element->type) {
        case AST_ELEMENT_TYPE_INTEGER: {
            struct type* type
                = allocator_allocate(context->allocator, sizeof(struct type));
            type->type      = TYPE_TYPE_BASE;
            type->base.name = from_cstr("Int");
            return type;
        }
        case AST_ELEMENT_TYPE_STRING: {
            struct type* type
                = allocator_allocate(context->allocator, sizeof(struct type));
            type->type      = TYPE_TYPE_BASE;
            type->base.name = from_cstr("Str");
            return type;
        }
        case AST_ELEMENT_TYPE_PROPERTY_ACCESS:
            struct type* type = nullptr;
            vector_foreach(element->property_access.ids, id) {
                if (type == nullptr) {
                    type = type_env_lookup(env, id);
                } else {
                    vector_foreach(type->properties, prop) {
                        if (strings_equal(prop->name, id)) {
                            type = prop->type;
                            break;
                        }
                    }
                    if (type->type == TYPE_TYPE_VAR) {
                        auto search_result = hashmap_string_type_find(context->types, type->var.name);
                        assert(search_result.found);
                        type = search_result.value;
                    }
                }
            }
            return type;
        case AST_ELEMENT_TYPE_PROPERTIES:
            return nullptr;
    }
    vix_unreachable();
}

static struct typecheck_env
new_typecheck_env(struct typecheck_env parent[static const 1]) {
    return (struct typecheck_env){
        .parent = parent,
        .names  = hashmap_string_id_new(parent->names.allocator),
    };
}

static usize
typecheck_env_find(struct typecheck_env env, struct string const name) {
    auto search_result = hashmap_string_id_find(env.names, name);
    if (search_result.found) {
        return search_result.value;
    } else {
        return typecheck_env_find(*env.parent, name);
    }
}

void
typecheck_init_properties(
    struct allocator allocator[static const 1],
    struct object_graph object_graph[static const 1],
    struct vector_ast_property_ptr const properties,
    struct typecheck_env env[static const 1]
) {
    auto new_env = new_typecheck_env(env);
    vector_foreach(properties, prop) {
        hashmap_string_id_add(&new_env.names, prop->name, prop->id);
    }

    vector_foreach(properties, prop) {
        graph_add_function(allocator, object_graph, prop->id);
        if (prop->value->type == AST_ELEMENT_TYPE_PROPERTIES) {
            vector_foreach(prop->value->properties, p) {
                graph_add_edge(allocator, object_graph, p->id, prop->id);
            }
            typecheck_init_properties(
                allocator, object_graph, prop->value->properties, &new_env
            );
        } else if (prop->value->type == AST_ELEMENT_TYPE_PROPERTY_ACCESS) {
            auto name = vector_string_head(prop->value->property_access.ids);
            auto id   = typecheck_env_find(new_env, name);
            graph_add_edge(allocator, object_graph, id, prop->id);
        }
    }
}
