#pragma once

#include "defs.h"

typedef struct {
    void* (*allocate)(i64 size, void* context);
    void* (*reallocate)(void* ptr, i64 old_size, i64 new_size, void* context);
    void (*free)(void* ptr, i64 size, void* context);
    void* context;
} allocator_t;

typedef struct {
    u8* buffer;
    i64 length;
    i64 previous_offset;
    i64 current_offset;
} arena_t;

static arena_t arena_init(void* buffer, i64 buffer_length) {
    return (arena_t){
        .buffer          = buffer,
        .length          = buffer_length,
        .current_offset  = 0,
        .previous_offset = 0,
    };
}

void* arena_allocate_align(arena_t* arena, i64 size, i64 align);
void* arena_allocate(arena_t* arena, i64 size);

void* arena_reallocate_align(
    arena_t* arena, void* old_memory, i64 old_size, i64 new_size, i64 align
);

void* arena_reallocate(
    arena_t* arena, void* old_memory, i64 old_size, i64 new_size
);

static void arena_free(arena_t* arena) {
    arena->current_offset  = 0;
    arena->previous_offset = 0;
}

static void empty_free(void* ptr, i64 size, void* context) {
    (void) ptr;
    (void) size;
    (void) context;
}

static void* arena_alloc(i64 size, void* context) {
    return arena_allocate((arena_t*) context, size);
}

static void* arena_realloc(
    void* ptr, i64 old_size, i64 new_size, void* context
) {
    return arena_reallocate((arena_t*) context, ptr, old_size, new_size);
}

#define init_arena_allocator(a)                     \
    (allocator_t) {                                 \
        arena_alloc, arena_realloc, empty_free, (a) \
    }
