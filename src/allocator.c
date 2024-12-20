#include "allocator.h"

#include <assert.h>
#include <string.h>

bool
is_power_of_two(uptr x) {
    return (x & (x - 1)) == 0;
}

uptr
align_forward(uptr ptr, usize align) {
    assert(is_power_of_two(align));

    uptr modulo = ptr & (align - 1);

    if (modulo != 0) {
        return ptr + align - modulo;
    } else {
        return ptr;
    }
}

struct arena
arena_init(void* backing_buffer, usize backing_buffer_length) {
    return (struct arena){
        .buffer          = backing_buffer,
        .buffer_length   = backing_buffer_length,
        .previous_offset = 0,
        .current_offset  = 0,
    };
}

void*
arena_allocate_align(struct arena* arena, usize size, usize align) {
    uptr curr_ptr = (uptr) arena->buffer + (uptr) arena->current_offset;
    uptr offset   = align_forward(curr_ptr, align);
    offset -= (uptr) arena->buffer;

    if (offset + size > arena->buffer_length) {
        return nullptr;
    }

    u8* ptr                = &arena->buffer[offset];
    arena->previous_offset = offset;
    arena->current_offset  = offset + size;

    memset(ptr, 0, size);
    return ptr;
}

void*
arena_allocate(struct arena* arena, size_t size) {
    return arena_allocate_align(arena, size, DEFAULT_ALIGNMENT);
}

void*
arena_resize_align(
    struct arena* arena, void* old_memory, usize old_size, usize new_size,
    usize align
) {
    u8* old_mem = (u8*) old_memory;

    assert(is_power_of_two(align));

    if (old_mem == NULL || old_size == 0) {
        return arena_allocate_align(arena, new_size, align);
    }

    if (arena->buffer <= old_mem
        && old_mem < arena->buffer + arena->buffer_length) {
        if (arena->buffer + arena->previous_offset == old_mem) {
            arena->current_offset = arena->previous_offset + new_size;
            if (new_size > old_size) {
                memset(
                    &arena->buffer[arena->current_offset], 0,
                    new_size - old_size
                );
            }
            return old_memory;
        } else {
            void* new_memory = arena_allocate_align(arena, new_size, align);
            size_t copy_size = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }
    } else {
        assert(0 && "Memory is out of bounds of the buffer in this arena");
        return nullptr;
    }
}

void*
arena_resize(
    struct arena* arena, void* old_memory, size_t old_size, size_t new_size
) {
    return arena_resize_align(
        arena, old_memory, old_size, new_size, DEFAULT_ALIGNMENT
    );
}

void
arena_free_all(struct arena* arena) {
    arena->buffer_length  = 0;
    arena->current_offset = 0;
}
