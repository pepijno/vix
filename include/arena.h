#pragma once

#include "allocator.h"
#include "defs.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

struct arena {
    u8* const buffer;
    usize buffer_length;
    usize previous_offset;
    usize current_offset;
};

struct arena arena_init(
    usize const backing_buffer_length,
    u8 backing_buffer[static const backing_buffer_length]
);
struct allocator arena_allocator_init(struct arena arena[static const 1]);
void arena_free_all(struct arena arena[static const 1]);
