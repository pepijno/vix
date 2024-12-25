#pragma once

#include "defs.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

struct allocator {
    void* (*allocate)(void* context, usize size);
    void* (*resize)(
        void* context, void* old_memory, usize old_size, usize new_size
    );
    void (*free)(void* context, void* memory, usize size);
    void* context;
};

void* allocator_allocate(struct allocator* allocator, usize size);
void* allocator_resize(
    struct allocator* allocator, void* old_memory, usize old_size,
    usize new_size
);
void allocator_free(struct allocator* allocator, void* memory, usize size);
