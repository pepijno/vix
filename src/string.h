#pragma once

#include "allocator.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct str_t {
    char data[256];
    size_t length;
} str_t;

#define str_fmt     "%.*s"
#define str_args(s) (int) (s).length, (s).data

static inline char str_at(str_t str, size_t index) {
    return str.data[index];
}

static inline str_t str_new_length(char* str, size_t length) {
    assert(length <= 256);
    str_t s = (str_t){
        .length = length,
    };
    memcpy(s.data, str, length);
    return s;
}

static inline str_t str_new_empty() {
    return str_new_length(nullptr, 0);
}

static inline str_t str_new(char* string) {
    return str_new_length(string, strlen(string));
}

static inline bool str_equal(str_t str1, str_t str2) {
    if (str1.length != str2.length) {
        return false;
    }
    // if (str1.data == nullptr) {
    //     return true;
    // }
    return memcmp(str1.data, str2.data, str1.length) == 0;
}

typedef struct str_buffer_t {
    char* data;
    size_t length;
    size_t capacity;
    allocator_t* allocator;
} str_buffer_t;

static void str_buffer_ensure_capacity(
    str_buffer_t str_buffer[static 1], size_t add_length
) {
    size_t available = str_buffer->capacity - str_buffer->length;
    if (available >= add_length) {
        return;
    }
    size_t length          = str_buffer->length;
    size_t required_length = length + add_length;
    size_t new_length      = 2 * required_length;
    allocator_t* allocator = str_buffer->allocator;
    void* new_data         = allocator->reallocate(
        str_buffer->data, sizeof(char) * length, sizeof(char) * new_length,
        allocator->context
    );
    if (!new_data) {
        vix_unreachable();
    } else {
        str_buffer->data = new_data;
    }
    str_buffer->capacity = new_length;
}

static str_buffer_t str_buffer_new(
    allocator_t allocator[static 1], size_t capacity
) {
    str_buffer_t buffer = {
        .data      = nullptr,
        .length    = 0,
        .capacity  = 0,
        .allocator = allocator,
    };
    str_buffer_ensure_capacity(&buffer, capacity);
    return buffer;
}

static void str_buffer_reset(str_buffer_t str_buffer[static 1]) {
    str_buffer->length   = 0;
    str_buffer->capacity = 0;
}

static str_t str_buffer_str(str_buffer_t str_buffer[static 1]) {
    str_t str = (str_t){
        .length = str_buffer->length,
    };
    memcpy(str.data, str_buffer->data, str_buffer->length);
    return str;
}

static void str_buffer_append_char(str_buffer_t str_buffer[static 1], char c) {
    str_buffer_ensure_capacity(str_buffer, 1);
    str_buffer->data[str_buffer->length] = c;
    str_buffer->length += 1;
}

static void str_buffer_append(str_buffer_t str_buffer[static 1], str_t str) {
    str_buffer_ensure_capacity(str_buffer, str.length);
    memcpy(str_buffer->data + str_buffer->length, str.data, str.length);
    str_buffer->length += str.length;
}

static int str_buffer_vprintf(
    str_buffer_t buffer[static 1], str_t format, va_list ap
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

static int str_buffer_printf(str_buffer_t buffer[static 1], str_t format, ...) {
    va_list ap;
    va_start(ap, format);
    int length = str_buffer_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}

static int str_buffer_append_vprintf(
    str_buffer_t buffer[static 1], str_t format, va_list ap
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

// __attribute__((format(printf, 2, 3)))
static int str_buffer_append_printf(
    str_buffer_t buffer[static 1], str_t format, ...
) {
    va_list ap;
    va_start(ap, format);
    int length = str_buffer_append_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}
