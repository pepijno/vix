#pragma once

#include "defs.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

struct arena {
    u8* buffer;
    usize buffer_length;
    usize previous_offset;
    usize current_offset;
};

struct arena arena_init(void* backing_buffer, usize backing_buffer_length);
void* arena_allocate(struct arena* arena, usize size);
void* arena_resize(
    struct arena* arena, void* old_memory, usize old_size, usize new_size
);
void arena_free_all(struct arena* arena);
