#include "allocator.h"

void*
allocator_allocate(struct allocator* allocator, usize size) {
    return allocator->allocate(allocator->context, size);
}
void*
allocator_resize(
    struct allocator* allocator, void* old_memory, usize old_size,
    usize new_size
) {
    return allocator->resize(
        allocator->context, old_memory, old_size, new_size
    );
}
void
allocator_free(struct allocator* allocator, void* memory, usize size) {
    allocator->free(allocator->context, memory, size);
}
