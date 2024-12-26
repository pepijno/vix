#include "allocator.h"

void*
allocator_allocate(
    struct allocator allocator[static const 1], usize const size
) {
    return allocator->allocate(allocator->context, size);
}
void*
allocator_resize(
    struct allocator allocator[static const 1], void* const old_memory,
    usize const old_size, usize const new_size
) {
    return allocator->resize(
        allocator->context, old_memory, old_size, new_size
    );
}
void
allocator_free(
    struct allocator allocator[static const 1], void const* const memory,
    usize const size
) {
    allocator->free(allocator->context, memory, size);
}
