#pragma once

#include "allocator.h"
#include "defs.h"
#include "vector.h"

#include <assert.h>
#include <string.h>

#define HASHMAP_NAME(__name)      CONCAT(hashmap_, __name)
#define KV_PAIR_NAME(__name)      CONCAT(kv_pair_, __name)
#define KV_PAIR_HASH_NAME(__name) CONCAT(kv_pair_hash_, __name)

#define HASHMAP_DEFS(__key_type, __value_type, __name)                    \
    struct KV_PAIR_NAME(__name) {                                         \
        __key_type key;                                                   \
        __value_type value;                                               \
    };                                                                    \
    struct KV_PAIR_HASH_NAME(__name) {                                    \
        struct KV_PAIR_NAME(__name) pair;                                 \
        u32 hash;                                                         \
    };                                                                    \
                                                                          \
    struct HASHMAP_NAME(__name) {                                         \
        struct allocator* allocator;                                      \
        struct vector_sizes indices;                                      \
        usize capacity;                                                   \
        usize length;                                                     \
        struct KV_PAIR_HASH_NAME(__name) * data;                          \
    };                                                                    \
                                                                          \
    struct HASHMAP_NAME(__name)                                           \
        CONCAT(HASHMAP_NAME(__name), _new)(struct allocator * allocator); \
                                                                          \
    __value_type CONCAT(HASHMAP_NAME(__name), _add)(                      \
        struct HASHMAP_NAME(__name) * __name, __key_type key,             \
        __value_type value                                                \
    );                                                                    \
                                                                          \
    __value_type CONCAT(HASHMAP_NAME(__name), _find)(                     \
        struct HASHMAP_NAME(__name) * __name, __key_type key              \
    );                                                                    \
                                                                          \
    bool CONCAT(HASHMAP_NAME(__name), _contains)(                         \
        struct HASHMAP_NAME(__name) __name, __key_type key                \
    );

#define HASHMAP_IMPL(                                                        \
    __key_type, __value_type, __name, __hash_fn, __is_equals_fn              \
)                                                                            \
    struct HASHMAP_NAME(__name)                                              \
        CONCAT(HASHMAP_NAME(__name), _new)(struct allocator * allocator) {   \
        usize capacity                      = 32;                            \
        struct HASHMAP_NAME(__name) hashmap = {                              \
            .capacity  = capacity,                                           \
            .allocator = allocator,                                          \
            .length    = 0,                                                  \
            .indices   = vector_sizes_new(allocator),                        \
        };                                                                   \
        hashmap.data = allocator_allocate(                                   \
            allocator, sizeof(struct KV_PAIR_NAME(__name)) * capacity        \
        );                                                                   \
        return hashmap;                                                      \
    }                                                                        \
                                                                             \
    struct CONCAT(__name, __LINE__) {                                        \
        usize position;                                                      \
        bool overwrite;                                                      \
    };                                                                       \
                                                                             \
    static struct CONCAT(__name, __LINE__)                                   \
        CONCAT(HASHMAP_NAME(__name), _find_new_position)(                    \
            struct HASHMAP_NAME(__name) __name, u32 hash, __key_type key     \
        ) {                                                                  \
        auto position  = hash & ((__name).capacity - 1);                     \
        usize increase = 1;                                                  \
        auto el        = (__name).data[position];                            \
        while (el.hash != 0) {                                               \
            if (__is_equals_fn(el.pair.key, key)) {                          \
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
    __value_type CONCAT(HASHMAP_NAME(__name), _add)(                         \
        struct HASHMAP_NAME(__name) * __name, __key_type key,                \
        __value_type value                                                   \
    ) {                                                                      \
        if (4 * (__name)->length >= 3 * (__name)->capacity) {                \
            auto new_size = sizeof(struct KV_PAIR_HASH_NAME(__name))         \
                          * (__name)->capacity * 2;                          \
            struct KV_PAIR_HASH_NAME(__name)* new_data                       \
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
                    = CONCAT(HASHMAP_NAME(__name), _find_new_position)(      \
                        *(__name), element.hash, element.pair.key            \
                    );                                                       \
                memcpy(                                                      \
                    new_data + position.position, &element,                  \
                    sizeof(struct KV_PAIR_HASH_NAME(__name))                 \
                );                                                           \
                if (!position.overwrite) {                                   \
                    vector_sizes_add(&(__name)->indices, position.position); \
                }                                                            \
            }                                                                \
                                                                             \
            (__name)->data = new_data;                                       \
        }                                                                    \
        auto hash = __hash_fn(key);                                          \
        if (hash < 2) {                                                      \
            hash += 2;                                                       \
        }                                                                    \
        auto position = CONCAT(HASHMAP_NAME(__name), _find_new_position)(    \
            *(__name), hash, key                                             \
        );                                                                   \
        (__name)->data[position.position].pair.key   = key;                  \
        (__name)->data[position.position].pair.value = value;                \
        (__name)->data[position.position].hash       = hash;                 \
        (__name)->length += 1;                                               \
        if (!position.overwrite) {                                           \
            vector_sizes_add(&(__name)->indices, position.position);         \
        }                                                                    \
        return (__name)->data[position.position].pair.value;                 \
    }                                                                        \
                                                                             \
    __value_type CONCAT(HASHMAP_NAME(__name), _find)(                        \
        struct HASHMAP_NAME(__name) * __name, __key_type key                 \
    ) {                                                                      \
        auto hash = __hash_fn(key);                                          \
        if (hash < 2) {                                                      \
            hash += 2;                                                       \
        }                                                                    \
        auto position  = hash & ((__name)->capacity - 1);                    \
        usize increase = 1;                                                  \
        auto el        = (__name)->data[position];                           \
        while (el.hash != 0) {                                               \
            if (__is_equals_fn(el.pair.key, key)) {                          \
                return (__name)->data[position].pair.value;                  \
            }                                                                \
            position = (position + increase) & ((__name)->capacity - 1);     \
            el       = (__name)->data[position];                             \
            increase += 1;                                                   \
        }                                                                    \
        return CONCAT(HASHMAP_NAME(__name), _add)(                           \
            (__name), (key), (__value_type){}                                \
        );                                                                   \
    }                                                                        \
                                                                             \
    bool CONCAT(HASHMAP_NAME(__name), _contains)(                            \
        struct HASHMAP_NAME(__name) __name, __key_type key                   \
    ) {                                                                      \
        auto hash = __hash_fn(key);                                          \
        if (hash < 2) {                                                      \
            hash += 2;                                                       \
        }                                                                    \
        auto position  = hash & ((__name).capacity - 1);                     \
        usize increase = 1;                                                  \
        auto el        = (__name).data[position];                            \
        while (el.hash != 0) {                                               \
            if (__is_equals_fn(el.pair.key, key)) {                          \
                return true;                                                 \
            }                                                                \
            position = (position + increase) & ((__name).capacity - 1);      \
            el       = (__name).data[position];                              \
            increase += 1;                                                   \
        }                                                                    \
        return false;                                                        \
    }

#define hashmap_foreach(__hashmap, __name)                                   \
    auto CONCAT(_foreach_hashmap, __LINE__) = (__hashmap);                   \
    auto CONCAT(_foreach_vector, __LINE__)                                   \
        = CONCAT(_foreach_hashmap, __LINE__).indices;                        \
    auto CONCAT(_foreach_vector_end, __LINE__)                               \
        = CONCAT(_foreach_vector, __LINE__).data                             \
        + CONCAT(_foreach_vector, __LINE__).length;                          \
    auto CONCAT(_foreach_iterator, __LINE__)                                 \
        = CONCAT(_foreach_vector, __LINE__).data;                            \
    auto CONCAT(_foreach_hashmap_index, __LINE__)                            \
        = *CONCAT(_foreach_iterator, __LINE__);                              \
    for (auto __name = CONCAT(_foreach_hashmap, __LINE__)                    \
                           .data[CONCAT(_foreach_hashmap_index, __LINE__)]   \
                           .pair;                                            \
         CONCAT(_foreach_iterator, __LINE__)                                 \
                 < CONCAT(_foreach_vector_end, __LINE__)                     \
             ? (CONCAT(_foreach_hashmap_index, __LINE__)                     \
                = *CONCAT(_foreach_iterator, __LINE__),                      \
                __name = CONCAT(_foreach_hashmap, __LINE__)                  \
                             .data[CONCAT(_foreach_hashmap_index, __LINE__)] \
                             .pair,                                          \
                true)                                                        \
             : false;                                                        \
         CONCAT(_foreach_iterator, __LINE__) += 1)
