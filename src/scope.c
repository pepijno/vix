#include "scope.h"

#include "allocator.h"
#include "ast.h"
#include "hash.h"

#include <assert.h>

static u32
name_hash(u32 const init, struct string const string) {
    return fnv1a_s(init, string);
}

static void
scope_insert_bucket(
    struct scope* scope, struct scope_object* object, struct string const name
) {
    *scope->next = object;
    scope->next  = &object->lnext;

    u32 const hash               = name_hash(HASH_INIT, name);
    struct scope_object** bucket = &scope->buckets[hash % BUCKET_SIZE];
    if (*bucket != nullptr) {
        object->mnext = *bucket;
    }
    *bucket = object;
}

struct scope*
scope_push(struct arena* arena, struct scope** stack) {
    struct scope* new_scope = arena_allocate(arena, sizeof(struct scope));
    new_scope->next         = &new_scope->objects;
    new_scope->parent       = *stack;
    *stack                  = new_scope;
    return new_scope;
}

struct scope*
scope_pop(struct scope** stack) {
    struct scope* previous_scope = *stack;
    assert(previous_scope != nullptr);
    *stack = previous_scope->parent;
    return previous_scope;
}

void
scope_insert_property(
    struct arena* arena, struct scope* scope, struct ast_property* property
) {
    struct scope_object* scope_object
        = arena_allocate(arena, sizeof(struct scope_object));
    scope_object->type     = SCOPE_OBJECT_TYPE_PROPERTY;
    scope_object->property = property;
    scope_insert_bucket(scope, scope_object, property->name);
}

struct scope_object*
scope_lookup_name(struct scope* scope, struct string const name) {
    u32 const hash              = name_hash(HASH_INIT, name);
    struct scope_object* bucket = scope->buckets[hash % BUCKET_SIZE];
    while (bucket) {
        if (bucket->type == SCOPE_OBJECT_TYPE_PROPERTY) {
            if (strings_equal(bucket->property->name, name)) {
                return bucket;
            }
        }
        bucket = bucket->mnext;
    }
    if (scope->parent) {
        return scope_lookup_name(scope->parent, name);
    }
    return nullptr;
}
