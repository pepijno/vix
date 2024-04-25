#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static bool is_power_of_two(uptr x) {
    return (x & (x - 1)) == 0;
}

static uptr align_forward(uptr ptr, i64 align) {
    assert(is_power_of_two(align));

    uptr p      = ptr;
    uptr a      = (uintptr_t) align;
    uptr modulo = p & (a - 1);

    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}

void* arena_allocate_align(arena_t* arena, i64 size, i64 align) {
    uptr curr_ptr = (uptr) arena->buffer + (uptr) arena->current_offset;
    uptr offset   = align_forward(curr_ptr, align);
    offset -= (uptr) arena->buffer;

    // Check to see if the backing memory has space left
    if (offset + size <= arena->length) {
        void* ptr              = &arena->buffer[offset];
        arena->previous_offset = offset;
        arena->current_offset  = offset + size;

        // Zero new memory by default
        memset(ptr, 0, size);
        return ptr;
    }
    return nullptr;
}

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#endif

void* arena_allocate(arena_t* arena, i64 size) {
    return arena_allocate_align(arena, size, DEFAULT_ALIGNMENT);
}

void* arena_reallocate_align(
    arena_t* arena, void* old_memory, i64 old_size, i64 new_size, i64 align
) {
    u8* old_mem = (u8*) old_memory;

    assert(is_power_of_two(align));

    if (old_mem == nullptr || old_size == 0) {
        return arena_allocate_align(arena, new_size, align);
    } else if (arena->buffer <= old_mem && old_mem < arena->buffer + arena->length) {
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
            i64 copy_size    = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }

    } else {
        assert(0 && "Memory is out of bounds of the buffer in this arena");
        return nullptr;
    }
}

void* arena_reallocate(
    arena_t* arena, void* old_memory, i64 old_size, i64 new_size
) {
    return arena_reallocate_align(
        arena, old_memory, old_size, new_size, DEFAULT_ALIGNMENT
    );
}
