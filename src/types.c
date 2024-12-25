#include "types.h"

#include "allocator.h"
#include "ast.h"
#include "graph.h"
#include "hash.h"
#include "hashmap.h"
#include "parser.h"
#include "str.h"
#include "util.h"
#include "vector.h"

#include <assert.h>

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
HASHMAP_IMPL(usize, struct _ast_property*, properties, usize_hash, usize_equals)

HASHMAP_IMPL(struct string, usize, string_id, string_hash, strings_equal)

static struct string
new_type_name(struct type_context* context) {
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
create_type_var(struct type_context* context) {
    auto name = new_type_name(context);
    struct type* type
        = allocator_allocate(context->allocator, sizeof(struct type));
    type->type     = TYPE_TYPE_VAR;
    type->var.name = name;
    return type;
}

static struct type*
resolve(
    struct type_context* context, struct type* type, struct type** type_var
) {
    *type_var = nullptr;
    while (type->type == TYPE_TYPE_VAR) {
        auto it = hashmap_string_type_find(&context->types, type->var.name);

        if (it == nullptr) {
            *type_var = type;
            break;
        }

        type = it;
    }

    return type;
}

static void
bind(struct type_context* context, struct string name, struct type* type) {
    if (type->type == TYPE_TYPE_VAR && strings_equal(type->var.name, name)) {
        return;
    }
    hashmap_string_type_add(&context->types, name, type);
}

static void
unify(struct type_context* context, struct type* left, struct type* right) {
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
type_env_lookup(struct type_env* type_env, struct string name) {
    auto it = hashmap_string_type_find(&type_env->names, name);
    if (it != nullptr) {
        return it;
    } else if (type_env->parent != nullptr) {
        return type_env_lookup(type_env->parent, name);
    } else {
        return nullptr;
    }
}

void
env_bind(struct type_env* type_env, struct string name, struct type* type) {
    hashmap_string_type_add(&type_env->names, name, type);
}

static struct type_env*
new_scope(struct type_env* type_env) {
    struct type_env* new_env
        = allocator_allocate(type_env->allocator, sizeof(struct type_env));
    new_env->allocator = type_env->allocator;
    new_env->parent    = type_env;
    new_env->names     = hashmap_string_type_new(type_env->allocator);
    return new_env;
}

void
typecheck_properties_first(
    struct type_context* context, struct type_env* env, struct string name,
    struct _ast_element* element
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
    struct type_context* context, struct type_env* env,
    struct _ast_element* element
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
    struct type_context* context, struct type_env* env,
    struct _ast_element* element
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
        case AST_ELEMENT_TYPE_ID:
            return type_env_lookup(env, element->id.id);
        case AST_ELEMENT_TYPE_PROPERTIES:
            return nullptr;
    }
    vix_unreachable();
}

static struct typecheck_env
new_typecheck_env(struct typecheck_env* parent) {
    return (struct typecheck_env){
        .parent = parent,
        .names  = hashmap_string_id_new(parent->names.allocator),
    };
}

static usize
typecheck_env_find(struct typecheck_env env, struct string const name) {
    if (hashmap_string_id_contains(env.names, name)) {
        return hashmap_string_id_find(&env.names, name);
    } else {
        return typecheck_env_find(*env.parent, name);
    }
}

void
typecheck_init_properties(
    struct allocator* allocator, struct object_graph* object_graph,
    struct vector_ast_property_ptr properties, struct typecheck_env* env
) {
    auto new_env = new_typecheck_env(env);
    vector_foreach(properties, prop) {
        hashmap_string_id_add(&new_env.names, prop->name, prop->id);
    }

    vector_foreach(properties, prop) {
        graph_add_function(allocator, object_graph, prop->id);
        if (prop->value->type == AST_ELEMENT_TYPE_PROPERTIES) {
            vector_foreach(prop->value->properties, p) {
                if (p->value->type == AST_ELEMENT_TYPE_PROPERTIES
                    || p->value->type == AST_ELEMENT_TYPE_ID) {
                    graph_add_edge(allocator, object_graph, p->id, prop->id);
                }
            }
            typecheck_init_properties(
                allocator, object_graph, prop->value->properties, &new_env
            );
        } else if (prop->value->type == AST_ELEMENT_TYPE_ID) {
            auto name = prop->value->id.id;
            auto id   = typecheck_env_find(new_env, name);
            graph_add_edge(allocator, object_graph, id, prop->id);
        }
    }
}
