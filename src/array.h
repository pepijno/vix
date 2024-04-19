#pragma once

#include "allocator.h"

typedef struct {
    size_t item_size;
    size_t length;
    size_t capacity;
    allocator_t* allocator;
} array_header_t;

static void* array_init(allocator_t* allocator, size_t const item_size) {
    void* ptr         = 0;
    size_t const size = 4 * item_size + sizeof(array_header_t);
    array_header_t* header =
        (array_header_t*) allocator->allocate(size, allocator->context);

    if (header) {
        header->item_size = item_size;
        header->length    = 0;
        header->capacity  = 4;
        header->allocator = allocator;
        ptr               = header + 1;
    }

    return ptr;
}

#define array(T, a) array_init((a), sizeof(T));

#define array_header(array) ((array_header_t*) (array) -1)

#define array_capacity(array) ((array) ? array_header(array)->capacity : 0)
#define array_length(array) \
    ((array) ? (ptrdiff_t) array_header(array)->length : (ptrdiff_t) 0)
#define array_length_unsigned(array) \
    ((array) ? array_header(array)->length : (size_t) 0)

static void* array_grow(
    void* array, size_t const add_length, size_t const capacity
) {
    array_header_t* header = array_header(array);
    size_t const minimum_length   = header->length + add_length;

    size_t minimum_capacity = capacity;
    // compute the minimum capacity needed
    if (minimum_length > minimum_capacity) {
        minimum_capacity = minimum_length;
    }

    if (minimum_capacity <= header->capacity) {
        return header + 1;
    }

    // increase needed capacity to guarantee O(1) amortized
    if (minimum_capacity < 2 * header->capacity) {
        minimum_capacity = 2 * header->capacity;
    } else if (minimum_capacity < 4) {
        minimum_capacity = 4;
    }

    size_t const item_size = (array) ? header->item_size : 0;
    size_t const size      = item_size * minimum_capacity + sizeof(*header);
    size_t const old_size =
        sizeof(*header) + header->length * header->item_size;

    array_header_t* new_header =
        (array_header_t*) header->allocator->reallocate(
            header, old_size, size, header->allocator->context
        );

    if (new_header) {
        new_header->allocator->free(
            header, old_size, new_header->allocator->context
        );
        new_header->capacity = minimum_capacity;
        header               = new_header + 1;
    } else {
        header = 0;
    }

    return header;
}

#define array_push(a, v)                                              \
    ((a) = array_grow((a), 1, 0), (a)[array_header(a)->length] = (v), \
     &(a)[array_header(a)->length++])
#define array_last(a) ((a)[array_header(a)->length - 1])
