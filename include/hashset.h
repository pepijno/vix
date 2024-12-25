#pragma once

#include "allocator.h"
#include "defs.h"
#include "vector.h"

#include <assert.h>

#define HASHSET_NAME(__name)            CONCAT(hashset_, __name)
#define HASHSET_VALUE_HASH_PAIR(__name) CONCAT(value_hash_pair_, __name)

#define HASHSET_STRUCT(__type, __name)                 \
    struct HASHSET_VALUE_HASH_PAIR(__name) {           \
        __type value;                                  \
        u32 hash;                                      \
    };                                                 \
                                                       \
    struct HASHSET_NAME(__name) {                      \
        struct allocator* allocator;                   \
        struct vector_sizes indices;                   \
        usize capacity;                                \
        usize length;                                  \
        struct HASHSET_VALUE_HASH_PAIR(__name) * data; \
    };

#define HASHSET_NEW_DEF(__name) \
    struct HASHSET_NAME(__name) \
        CONCAT(HASHSET_NAME(__name), _new)(struct allocator * allocator);

#define HASHSET_ADD_DEF(__type, __name)                      \
    void CONCAT(HASHSET_NAME(__name), _add)(                 \
        struct HASHSET_NAME(__name) * __name, __type element \
    );

#define HASHSET_CONTAINS_DEF(__type, __name)               \
    bool CONCAT(HASHSET_NAME(__name), _contains)(          \
        struct HASHSET_NAME(__name) __name, __type element \
    );

#define HASHSET_COPY_DEF(__type, __name) \
    struct HASHSET_NAME(__name           \
    ) CONCAT(HASHSET_NAME(__name), _copy)(struct HASHSET_NAME(__name) __name);

#define HASHSET_DEFS(__type, __name)     \
    HASHSET_STRUCT(__type, __name)       \
    HASHSET_NEW_DEF(__name)              \
    HASHSET_ADD_DEF(__type, __name)      \
    HASHSET_CONTAINS_DEF(__type, __name) \
    HASHSET_COPY_DEF(__type, __name)

#define HASHSET_NEW_IMPL(__type, __name)                                   \
    struct HASHSET_NAME(__name)                                            \
        CONCAT(HASHSET_NAME(__name), _new)(struct allocator * allocator) { \
        usize capacity                      = 32;                          \
        struct HASHSET_NAME(__name) hashset = {                            \
            .capacity  = capacity,                                         \
            .allocator = allocator,                                        \
            .length    = 0,                                                \
            .indices   = vector_sizes_new(allocator),                      \
        };                                                                 \
        hashset.data = allocator_allocate(                                 \
            allocator,                                                     \
            sizeof(struct HASHSET_VALUE_HASH_PAIR(__name)) * capacity      \
        );                                                                 \
        return hashset;                                                    \
    }

#define HASHSET_ADD_IMPL(__type, __name, __hash_fn, __is_equals_fn)          \
    struct CONCAT(__name, __LINE__) {                                        \
        usize position;                                                      \
        bool overwrite;                                                      \
    };                                                                       \
                                                                             \
    static struct CONCAT(__name, __LINE__)                                   \
        CONCAT(HASHSET_NAME(__name), _find_new_position)(                    \
            struct HASHSET_NAME(__name) __name, u32 hash, __type element     \
        ) {                                                                  \
        auto position  = hash & ((__name).capacity - 1);                     \
        usize increase = 1;                                                  \
        auto el        = (__name).data[position];                            \
        while (el.hash != 0) {                                               \
            if (__is_equals_fn(el.value, element)) {                         \
                return (struct CONCAT(__name, __LINE__)){                    \
                    .position  = position,                                   \
                    .overwrite = true,                                       \
                };                                                           \
            }                                                                \
            position = (position + increase) & ((__name).capacity - 1);      \
            el       = (__name).data[position];                              \
            increase += 1;                                                   \
        }                                                                    \
        return (struct CONCAT(__name, __LINE__)){                            \
            .position  = position,                                           \
            .overwrite = false,                                              \
        };                                                                   \
    }                                                                        \
                                                                             \
    void CONCAT(HASHSET_NAME(__name), _add)(                                 \
        struct HASHSET_NAME(__name) * __name, __type new_element             \
    ) {                                                                      \
        if (4 * (__name)->length >= 3 * (__name)->capacity) {                \
            auto new_size = sizeof(struct HASHSET_VALUE_HASH_PAIR(__name))   \
                          * (__name)->capacity * 2;                          \
            struct HASHSET_VALUE_HASH_PAIR(__name)* new_data                 \
                = allocator_allocate((__name)->allocator, new_size);         \
            vector_sizes_clear(&(__name)->indices);                          \
            auto old_capacity = (__name)->capacity;                          \
            (__name)->capacity *= 2;                                         \
                                                                             \
            for (usize i = 0; i < old_capacity; i += 1) {                    \
                auto element = (__name)->data[i];                            \
                if (element.hash == 0) {                                     \
                    continue;                                                \
                }                                                            \
                auto position                                                \
                    = CONCAT(HASHSET_NAME(__name), _find_new_position)(      \
                        *(__name), element.hash, element.value               \
                    );                                                       \
                memcpy(                                                      \
                    new_data + position.position, &element,                  \
                    sizeof(struct HASHSET_VALUE_HASH_PAIR(__name))           \
                );                                                           \
                if (!position.overwrite) {                                   \
                    vector_sizes_add(&(__name)->indices, position.position); \
                }                                                            \
            }                                                                \
                                                                             \
            (__name)->data = new_data;                                       \
        }                                                                    \
        auto hash = __hash_fn(new_element);                                  \
        if (hash < 2) {                                                      \
            hash += 2;                                                       \
        }                                                                    \
        auto position = CONCAT(HASHSET_NAME(__name), _find_new_position)(    \
            *(__name), hash, new_element                                     \
        );                                                                   \
        (__name)->data[position.position].hash  = hash;                      \
        (__name)->data[position.position].value = new_element;               \
        (__name)->length += 1;                                               \
        if (!position.overwrite) {                                           \
            vector_sizes_add(&(__name)->indices, position.position);         \
        }                                                                    \
    }

#define HASHSET_CONTAINS_IMPL(__type, __name, __hash_fn, __is_equals_fn) \
    bool CONCAT(HASHSET_NAME(__name), _contains)(                        \
        struct HASHSET_NAME(__name) __name, __type element               \
    ) {                                                                  \
        auto hash = __hash_fn(element);                                  \
        if (hash < 2) {                                                  \
            hash += 2;                                                   \
        }                                                                \
        auto position  = hash & ((__name).capacity - 1);                 \
        usize increase = 1;                                              \
        auto el        = (__name).data[position];                        \
        while (el.hash != 0) {                                           \
            if (__is_equals_fn(el.value, element)) {                     \
                return true;                                             \
            }                                                            \
            position = (position + increase) & ((__name).capacity - 1);  \
            el       = (__name).data[position];                          \
            increase += 1;                                               \
        }                                                                \
        return false;                                                    \
    }

#define HASHSET_COPY_IMPL(__type, __name)                                                            \
    struct HASHSET_NAME(__name)                                                                      \
        CONCAT(HASHSET_NAME(__name), _copy)(struct HASHSET_NAME(__name) __name                       \
        ) {                                                                                          \
        struct HASHSET_NAME(__name) new_set = (__name);                                              \
        new_set.data                        = allocator_allocate(                                    \
            new_set.allocator,                                                \
            sizeof(struct HASHSET_VALUE_HASH_PAIR(__name)) * new_set.capacity \
        );                                                                    \
        memcpy(                                                                                      \
            new_set.data, (__name).data,                                                             \
            sizeof(struct HASHSET_VALUE_HASH_PAIR(__name)) * new_set.capacity                        \
        );                                                                                           \
        return new_set;                                                                              \
    }

#define HASHSET_IMPL(__type, __name, __hash_fn, __is_equals_fn)      \
    HASHSET_NEW_IMPL(__type, __name)                                 \
    HASHSET_ADD_IMPL(__type, __name, __hash_fn, __is_equals_fn)      \
    HASHSET_CONTAINS_IMPL(__type, __name, __hash_fn, __is_equals_fn) \
    HASHSET_COPY_IMPL(__type, __name)

#define hashset_foreach(__hashset, __name)                                   \
    auto CONCAT(_foreach_hashset, __LINE__) = (__hashset);                   \
    auto CONCAT(_foreach_vector, __LINE__)                                   \
        = CONCAT(_foreach_hashset, __LINE__).indices;                        \
    auto CONCAT(_foreach_vector_end, __LINE__)                               \
        = CONCAT(_foreach_vector, __LINE__).data                             \
        + CONCAT(_foreach_vector, __LINE__).length;                          \
    auto CONCAT(_foreach_iterator, __LINE__)                                 \
        = CONCAT(_foreach_vector, __LINE__).data;                            \
    auto CONCAT(_foreach_hashset_index, __LINE__)                            \
        = *CONCAT(_foreach_iterator, __LINE__);                              \
    for (auto __name = CONCAT(_foreach_hashset, __LINE__)                    \
                           .data[CONCAT(_foreach_hashset_index, __LINE__)]   \
                           .value;                                           \
         CONCAT(_foreach_iterator, __LINE__)                                 \
                 < CONCAT(_foreach_vector_end, __LINE__)                     \
             ? (CONCAT(_foreach_hashset_index, __LINE__)                     \
                = *CONCAT(_foreach_iterator, __LINE__),                      \
                __name = CONCAT(_foreach_hashset, __LINE__)                  \
                             .data[CONCAT(_foreach_hashset_index, __LINE__)] \
                             .value,                                         \
                true)                                                        \
             : false;                                                        \
         CONCAT(_foreach_iterator, __LINE__) += 1)
