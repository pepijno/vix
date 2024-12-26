#include "arena.h"

#include "allocator.h"

#include <assert.h>
#include <string.h>

static bool
is_power_of_two(uptr const x) {
    return (x & (x - 1)) == 0;
}

static uptr
align_forward(uptr const ptr, usize const align) {
    assert(is_power_of_two(align));

    uptr const modulo = ptr & (align - 1);

    if (modulo != 0) {
        return ptr + align - modulo;
    } else {
        return ptr;
    }
}

static void*
arena_allocate_align(
    struct arena arena[static const 1], usize const size, usize const align
) {
    uptr const curr_ptr = (uptr) arena->buffer + (uptr) arena->current_offset;
    uptr offset         = align_forward(curr_ptr, align);
    offset -= (uptr) arena->buffer;

    if (offset + size > arena->buffer_length) {
        return nullptr;
    }

    u8* const ptr          = &arena->buffer[offset];
    arena->previous_offset = offset;
    arena->current_offset  = offset + size;

    memset(ptr, 0, size);
    return ptr;
}

static void*
arena_allocate(void* const context, size_t const size) {
    return arena_allocate_align(
        (struct arena*) context, size, DEFAULT_ALIGNMENT
    );
}

static void*
arena_resize_align(
    struct arena arena[static const 1], void* const old_memory,
    usize const old_size, usize const new_size, usize const align
) {
    u8* const old_mem = (u8*) old_memory;

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
            void* const new_memory
                = arena_allocate_align(arena, new_size, align);
            usize const copy_size = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }
    } else {
        assert(0 && "Memory is out of bounds of the buffer in this arena");
        return nullptr;
    }
}

static void*
arena_resize(
    void* const context, void* const old_memory, usize const old_size,
    usize const new_size
) {
    return arena_resize_align(
        (struct arena*) context, old_memory, old_size, new_size,
        DEFAULT_ALIGNMENT
    );
}

static void
arena_free(void* const context, void const* const memory, usize const size) {
    (void)context;
    (void)memory;
    (void)size;
}

struct arena
arena_init(
    usize const backing_buffer_length,
    u8 backing_buffer[static const backing_buffer_length]
) {
    return (struct arena){
        .buffer          = backing_buffer,
        .buffer_length   = backing_buffer_length,
        .previous_offset = 0,
        .current_offset  = 0,
    };
}

struct allocator
arena_allocator_init(struct arena arena[static const 1]) {
    return (struct allocator){
        .allocate = arena_allocate,
        .resize   = arena_resize,
        .free     = arena_free,
        .context  = arena,
    };
}

void
arena_free_all(struct arena arena[static const 1]) {
    arena->buffer_length  = 0;
    arena->current_offset = 0;
}
