#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void* (*allocate)(size_t const size, void* context);
    void* (*reallocate)(
        void* ptr, size_t const old_size, size_t const new_size, void* context
    );
    void (*free)(void* ptr, size_t const size, void* context);
    void* context;
} allocator_t;

typedef struct region_t region_t;
struct region_t {
    region_t* next;
    size_t count;
    size_t capacity;
    uintptr_t data[];
};

#define REGION_DEFAULT_CAPACITY (8 * 1024)

static region_t* new_region(size_t const capacity) {
    size_t const size_bytes = sizeof(region_t) + sizeof(uintptr_t) * capacity;
    region_t* region        = malloc(size_bytes);
    assert(region);
    region->next     = NULL;
    region->count    = 0;
    region->capacity = capacity;
    return region;
}

typedef struct {
    region_t* begin;
    region_t* end;
} arena_t;

static void free_region(region_t* region) {
    free(region);
}

static void* arena_allocate(size_t const size_bytes, void* const context) {
    arena_t* const arena = (arena_t*) context;
    size_t const size =
        (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (arena->end == NULL) {
        assert(arena->begin == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size) {
            capacity = size;
        }
        arena->end   = new_region(capacity);
        arena->begin = arena->end;
    }

    while (arena->end->count + size > arena->end->capacity &&
           arena->end->next != NULL) {
        arena->end = arena->end->next;
    }

    if (arena->end->count + size > arena->end->capacity) {
        assert(arena->end->next == NULL);
        size_t capacity = REGION_DEFAULT_CAPACITY;
        if (capacity < size) {
            capacity = size;
        }
        arena->end->next = new_region(capacity);
        arena->end       = arena->end->next;
    }

    void* result = &arena->end->data[arena->end->count];
    arena->end->count += size;
    return result;
}

static void* arena_reallocate(
    void* const old_ptr, size_t const old_size, size_t const new_size,
    void* const context
) {
    if (new_size <= old_size) {
        return old_ptr;
    }
    void* new_ptr      = arena_allocate(new_size, context);
    char* new_ptr_char = new_ptr;
    char* old_ptr_char = old_ptr;
    memcpy(new_ptr_char, old_ptr_char, old_size);
    return new_ptr;
}

static void arena_reset(arena_t* arena) {
    for (region_t* region = arena->begin; region != NULL;
         region           = region->next) {
        region->count = 0;
    }

    arena->end = arena->begin;
}

static void arena_free(arena_t* a) {
    region_t* region = a->begin;
    while (region != nullptr) {
        region_t* r = region;
        region      = region->next;
        free_region(r);
    }
    a->begin = NULL;
    a->end   = NULL;
}

static void empty_free(void* ptr, size_t const size, void* context) {
    (void) ptr;
    (void) size;
    (void) context;
}

#define init_arena_allocator(a)                           \
    (allocator_t) {                                       \
        arena_allocate, arena_reallocate, empty_free, (a) \
    }
