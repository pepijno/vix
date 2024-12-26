#pragma once

#include "allocator.h"
#include "defs.h"

#include <assert.h>

#define VECTOR_NAME(__name) CONCAT(vector_, __name)

#define VECTOR_DEFS(__type, __name)                                      \
    struct VECTOR_NAME(__name) {                                         \
        struct allocator* allocator;                                     \
        usize length;                                                    \
        usize capacity;                                                  \
        __type* data;                                                    \
    };                                                                   \
                                                                         \
    struct VECTOR_NAME(__name)                                           \
        CONCAT(VECTOR_NAME(__name), _new)(struct allocator * allocator); \
                                                                         \
    void CONCAT(VECTOR_NAME(__name), _add)(                              \
        struct VECTOR_NAME(__name) * __name, __type element              \
    );                                                                   \
                                                                         \
    void CONCAT(VECTOR_NAME(__name), _clear)(                            \
        struct VECTOR_NAME(__name) * __name                              \
    );

#define VECTOR_IMPL(__type, __name)                                       \
    struct VECTOR_NAME(__name)                                            \
        CONCAT(VECTOR_NAME(__name), _new)(struct allocator * allocator) { \
        usize capacity                    = 32;                           \
        struct VECTOR_NAME(__name) vector = {                             \
            .capacity  = capacity,                                        \
            .allocator = allocator,                                       \
            .length    = 0,                                               \
        };                                                                \
        vector.data                                                       \
            = allocator_allocate(allocator, sizeof(__type) * capacity);   \
        return vector;                                                    \
    }                                                                     \
                                                                          \
    void CONCAT(VECTOR_NAME(__name), _add)(                               \
        struct VECTOR_NAME(__name) * __name, __type element               \
    ) {                                                                   \
        if ((__name)->length == (__name)->capacity) {                     \
            usize old_size = (__name)->capacity * sizeof(__type);         \
            usize new_size = old_size * 2;                                \
            (__name)->data = allocator_resize(                            \
                (__name)->allocator, (__name)->data, old_size, new_size   \
            );                                                            \
            (__name)->capacity *= 2;                                      \
        }                                                                 \
        (__name)->data[(__name)->length] = element;                       \
        (__name)->length += 1;                                            \
    }                                                                     \
                                                                          \
    void CONCAT(VECTOR_NAME(__name), _clear)(                             \
        struct VECTOR_NAME(__name) * __name                               \
    ) {                                                                   \
        (__name)->length = 0;                                             \
    }

#define vector_foreach(__vector, __name)                             \
    auto CONCAT(_foreach_vector, __LINE__) = (__vector);             \
    auto CONCAT(_foreach_vector_end, __LINE__)                       \
        = CONCAT(_foreach_vector, __LINE__).data                     \
        + CONCAT(_foreach_vector, __LINE__).length;                  \
    auto CONCAT(_foreach_iterator, __LINE__)                         \
        = CONCAT(_foreach_vector, __LINE__).data;                    \
    for (auto __name = *CONCAT(_foreach_iterator, __LINE__);         \
         CONCAT(_foreach_iterator, __LINE__)                         \
                 < CONCAT(_foreach_vector_end, __LINE__)             \
             ? (__name = *CONCAT(_foreach_iterator, __LINE__), true) \
             : false;                                                \
         CONCAT(_foreach_iterator, __LINE__) += 1)

VECTOR_DEFS(usize, sizes)
