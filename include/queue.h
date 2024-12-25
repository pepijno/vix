#pragma once

#include "allocator.h"
#include "defs.h"

#include <assert.h>
#include <string.h>

#define QUEUE_NAME(__name) CONCAT(queue_, __name)

#define QUEUE_DEF(__type, __name)                                        \
    struct QUEUE_NAME(__name) {                                          \
        struct allocator* allocator;                                     \
        usize head;                                                      \
        usize tail;                                                      \
        usize capacity;                                                  \
        usize length;                                                    \
        __type* data;                                                    \
    };                                                                   \
                                                                         \
    struct QUEUE_NAME(__name)                                            \
        CONCAT(QUEUE_NAME(__name), _new)(struct allocator * allocator);  \
                                                                         \
    void CONCAT(QUEUE_NAME(__name), _push)(                              \
        struct QUEUE_NAME(__name) * __name, __type value                 \
    );                                                                   \
    bool CONCAT(QUEUE_NAME(__name), _is_empty)(struct QUEUE_NAME(__name) \
                                                   __name);              \
    __type CONCAT(QUEUE_NAME(__name), _pop)(struct QUEUE_NAME(__name) * __name);

#define QUEUE_IMPL(__type, __name)                                            \
    struct QUEUE_NAME(__name)                                                 \
        CONCAT(QUEUE_NAME(__name), _new)(struct allocator * allocator) {      \
        usize capacity = 32;                                                  \
        return (struct QUEUE_NAME(__name)){                                   \
            .allocator = allocator,                                           \
            .head      = 0,                                                   \
            .tail      = 0,                                                   \
            .capacity  = capacity,                                            \
            .length    = 0,                                                   \
            .data = allocator_allocate(allocator, sizeof(__type) * capacity), \
        };                                                                    \
    }                                                                         \
                                                                              \
    void CONCAT(QUEUE_NAME(__name), _push)(                                   \
        struct QUEUE_NAME(__name) * __name, __type value                      \
    ) {                                                                       \
        if ((__name)->length == (__name)->capacity) {                         \
            usize old_size = (__name)->capacity * sizeof(__type);             \
            usize new_size = old_size * 2;                                    \
            (__name)->data = allocator_resize(                                \
                (__name)->allocator, (__name)->data, old_size, new_size       \
            );                                                                \
            (__name)->capacity *= 2;                                          \
        }                                                                     \
        (__name)->data[(__name)->head] = value;                               \
        (__name)->head = ((__name)->head + 1) & ((__name)->capacity - 1);     \
        (__name)->length += 1;                                                \
    }                                                                         \
                                                                              \
    bool CONCAT(QUEUE_NAME(__name), _is_empty)(struct QUEUE_NAME(__name)      \
                                                   __name) {                  \
        return (__name).length == 0;                                          \
    }                                                                         \
                                                                              \
    __type CONCAT(QUEUE_NAME(__name), _pop)(                                  \
        struct QUEUE_NAME(__name) * __name                                    \
    ) {                                                                       \
        __type value   = (__name)->data[(__name)->tail];                      \
        (__name)->tail = ((__name)->tail + 1) & ((__name)->capacity - 1);     \
        (__name)->length -= 1;                                                \
        return value;                                                         \
    }
