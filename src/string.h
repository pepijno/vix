#pragma once

#include "allocator.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct str_t {
    char const* data;
    size_t length;
};

#define str_fmt     "%.*s"
#define str_args(s) (int) (s).length, (s).data

static inline char str_at(struct str_t const str, size_t const index) {
    return str.data[index];
}

static inline struct str_t str_new_length(
    char const* const str, size_t const length
) {
    return (struct str_t){
        .data   = str,
        .length = length,
    };
}

static inline struct str_t str_new_empty() {
    return str_new_length(nullptr, 0);
}

static inline struct str_t str_new(char const* const string) {
    return str_new_length(string, strlen(string));
}

static inline bool str_equal(struct str_t const str1, struct str_t const str2) {
    if (str1.length != str2.length) {
        return false;
    }
    if (str1.data == nullptr) {
        return true;
    }
    return memcmp(str1.data, str2.data, str1.length) == 0;
}

struct str_buffer_t {
    char* data;
    size_t length;
    size_t capacity;
    struct allocator_t* allocator;
};

static void str_buffer_ensure_capacity(
    struct str_buffer_t str_buffer[static const 1], size_t const add_length
) {
    size_t const available = str_buffer->capacity;
    if (available >= add_length) {
        return;
    }
    size_t const length          = str_buffer->length;
    size_t const required_length = length + add_length;
    size_t const new_length       = 2 * required_length;
    struct allocator_t* allocator = str_buffer->allocator;
    str_buffer->data              = allocator->reallocate(
        str_buffer->data, sizeof(char) * length, sizeof(char) * new_length,
        allocator->context
    );
    // if (!new_data) {
    //     return NULL;
    // }
    str_buffer->capacity = new_length;
}

static struct str_buffer_t str_buffer_new(
    struct allocator_t allocator[static const 1], size_t const capacity
) {
    struct str_buffer_t buffer = {
        .data      = nullptr,
        .length    = 0,
        .capacity  = 0,
        .allocator = allocator,
    };
    str_buffer_ensure_capacity(&buffer, capacity);
    return buffer;
}

static void str_buffer_reset(struct str_buffer_t str_buffer[static const 1]) {
    str_buffer->length = 0;
}

static struct str_t str_buffer_str(
    struct str_buffer_t const str_buffer[static const 1]
) {
    char* data = str_buffer->allocator->allocate(
        sizeof(char) * str_buffer->length, str_buffer->allocator->context
    );
    memcpy(data, str_buffer->data, str_buffer->length);
    return (struct str_t){
        .data   = data,
        .length = str_buffer->length,
    };
}

static void str_buffer_append_char(
    struct str_buffer_t str_buffer[static const 1], char const c
) {
    str_buffer_ensure_capacity(str_buffer, 1);
    str_buffer->data[str_buffer->length] = c;
    str_buffer->length += 1;
}

static void str_buffer_append(
    struct str_buffer_t str_buffer[static const 1], struct str_t const str
) {
    str_buffer_ensure_capacity(str_buffer, str.length);
    memcpy(str_buffer->data + str_buffer->length, str.data, str.length);
    str_buffer->length += str.length;
}

static int str_buffer_vprintf(
    struct str_buffer_t buffer[static const 1], struct str_t const format,
    va_list ap
) {
    str_buffer_reset(buffer);

    va_list ap2;
    va_copy(ap2, ap);

    int length1 = vsnprintf(nullptr, 0, format.data, ap);
    assert(length1 >= 0);

    str_buffer_ensure_capacity(buffer, length1);

    int length2 = vsnprintf(buffer->data, length1 + 1, format.data, ap2);
    assert(length1 == length2);

    va_end(ap2);

    buffer->length = length1;

    return length1;
}

static int str_buffer_printf(
    struct str_buffer_t buffer[static const 1], struct str_t const format, ...
) {
    va_list ap;
    va_start(ap, format);
    int length = str_buffer_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}

static int str_buffer_append_vprintf(
    struct str_buffer_t buffer[static const 1], struct str_t const format,
    va_list ap
) {
    va_list ap2;
    va_copy(ap2, ap);

    int length1 = vsnprintf(nullptr, 0, format.data, ap);
    assert(length1 >= 0);

    str_buffer_ensure_capacity(buffer, length1);

    int length2 =
        vsnprintf(buffer->data + buffer->length, length1 + 1, format.data, ap2);
    assert(length1 == length2);

    va_end(ap2);

    buffer->length += length1;

    return length1;
}

static int str_buffer_append_printf(
    struct str_buffer_t buffer[static const 1], struct str_t const format, ...
) {
    va_list ap;
    va_start(ap, format);
    int length = str_buffer_append_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}
