#pragma once

#include "defs.h"

typedef struct {
    void* (*allocate)(i64 size, void* context);
    void* (*reallocate)(void* ptr, i64 old_size, i64 new_size, void* context);
    void (*free)(void* ptr, i64 size, void* context);
    void* context;
} Allocator;

typedef struct {
    u8* buffer;
    i64 length;
    i64 previous_offset;
    i64 current_offset;
} Arena;

static Arena arena_init(void* buffer, i64 buffer_length) {
    return (Arena){
        .buffer          = buffer,
        .length          = buffer_length,
        .current_offset  = 0,
        .previous_offset = 0,
    };
}

void* arena_allocate_align(Arena* arena, i64 size, i64 align);
void* arena_allocate(Arena* arena, i64 size);

void* arena_reallocate_align(
    Arena* arena, void* old_memory, i64 old_size, i64 new_size, i64 align
);

void* arena_reallocate(
    Arena* arena, void* old_memory, i64 old_size, i64 new_size
);

static void arena_free(Arena* arena) {
    arena->current_offset  = 0;
    arena->previous_offset = 0;
}

static void empty_free(void* ptr, i64 size, void* context) {
    (void) ptr;
    (void) size;
    (void) context;
}

static void* arena_alloc(i64 size, void* context) {
    return arena_allocate((Arena*) context, size);
}

static void* arena_realloc(
    void* ptr, i64 old_size, i64 new_size, void* context
) {
    return arena_reallocate((Arena*) context, ptr, old_size, new_size);
}

#define init_arena_allocator(a)                     \
    (Allocator) {                                 \
        arena_alloc, arena_realloc, empty_free, (a) \
    }
