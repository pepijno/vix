#pragma once

#include "defs.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

struct allocator {
    void* (*allocate)(void* const context, usize const size);
    void* (*resize)(
        void* const context, void* const old_memory, usize const old_size,
        usize const new_size
    );
    void (*const free)(
        void* const context, void const* const memory, usize const size
    );
    void* const context;
};

void* allocator_allocate(
    struct allocator allocator[static const 1], usize const size
);
void* allocator_resize(
    struct allocator allocator[static const 1], void* const old_memory,
    usize const old_size, usize const new_size
);
void allocator_free(
    struct allocator allocator[static const 1], void const* const memory,
    usize const size
);
