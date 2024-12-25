#pragma once

#include "allocator.h"
#include "defs.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

struct arena {
    u8* buffer;
    usize buffer_length;
    usize previous_offset;
    usize current_offset;
};

struct arena arena_init(void* backing_buffer, usize backing_buffer_length);
struct allocator arena_allocator_init(struct arena* arena);
void arena_free_all(struct arena* arena);
