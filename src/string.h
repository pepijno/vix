#pragma once

#include "allocator.h"
#include "defs.h"
#include "util.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    u8 data[256];
    i64 length;
} Str;

#define str_fmt     "%.*s"
#define str_args(s) (i32)(s).length, (s).data

static inline u8 str_at(Str str, i64 index) {
    return str.data[index];
}

static inline Str str_new_length(u8* str, i64 length) {
    assert(length <= 256);
    Str s = (Str){
        .length = length,
    };
    memcpy(s.data, str, length);
    return s;
}

static inline Str str_new_empty() {
    return str_new_length(nullptr, 0);
}

static inline Str str_new(u8* string) {
    return str_new_length(string, strlen((char*)string));
}

static inline bool str_equal(Str str1, Str str2) {
    if (str1.length != str2.length) {
        return false;
    }
    // if (str1.data == nullptr) {
    //     return true;
    // }
    return memcmp(str1.data, str2.data, str1.length) == 0;
}

typedef struct {
    u8* data;
    i64 length;
    i64 capacity;
    Allocator* allocator;
} StrBuffer;

static void str_buffer_ensure_capacity(
    StrBuffer str_buffer[static 1], i64 add_length
) {
    i64 available = str_buffer->capacity - str_buffer->length;
    if (available >= add_length) {
        return;
    }
    i64 length             = str_buffer->length;
    i64 required_length    = length + add_length;
    i64 new_length         = 2 * required_length;
    Allocator* allocator = str_buffer->allocator;
    void* new_data         = allocator->reallocate(
        str_buffer->data, sizeof(u8) * length, sizeof(u8) * new_length,
        allocator->context
    );
    if (!new_data) {
        vix_unreachable();
    } else {
        str_buffer->data = new_data;
    }
    str_buffer->capacity = new_length;
}

static StrBuffer str_buffer_new(
    Allocator allocator[static 1], i64 capacity
) {
    StrBuffer buffer = {
        .data      = nullptr,
        .length    = 0,
        .capacity  = 0,
        .allocator = allocator,
    };
    str_buffer_ensure_capacity(&buffer, capacity);
    return buffer;
}

static void str_buffer_reset(StrBuffer str_buffer[static 1]) {
    str_buffer->length   = 0;
    str_buffer->capacity = 0;
}

static Str str_buffer_str(StrBuffer str_buffer[static 1]) {
    Str str = (Str){
        .length = str_buffer->length,
    };
    memcpy(str.data, str_buffer->data, str_buffer->length);
    return str;
}

static void str_buffer_append_char(StrBuffer str_buffer[static 1], u8 c) {
    str_buffer_ensure_capacity(str_buffer, 1);
    str_buffer->data[str_buffer->length] = c;
    str_buffer->length += 1;
}

static void str_buffer_append(StrBuffer str_buffer[static 1], Str str) {
    str_buffer_ensure_capacity(str_buffer, str.length);
    memcpy(str_buffer->data + str_buffer->length, str.data, str.length);
    str_buffer->length += str.length;
}

static i32 str_buffer_vprintf(
    StrBuffer buffer[static 1], Str format, va_list ap
) {
    str_buffer_reset(buffer);

    va_list ap2;
    va_copy(ap2, ap);

    i32 length1 = vsnprintf(nullptr, 0, (char*) format.data, ap);
    assert(length1 >= 0);

    str_buffer_ensure_capacity(buffer, length1);

    i32 length2 =
        vsnprintf((char*) buffer->data, length1 + 1, (char*) format.data, ap2);
    assert(length1 == length2);

    va_end(ap2);

    buffer->length = length1;

    return length1;
}

static i32 str_buffer_printf(StrBuffer buffer[static 1], Str format, ...) {
    va_list ap;
    va_start(ap, format);
    i32 length = str_buffer_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}

static i32 str_buffer_append_vprintf(
    StrBuffer buffer[static 1], Str format, va_list ap
) {
    va_list ap2;
    va_copy(ap2, ap);

    i32 length1 = vsnprintf(nullptr, 0, (char*) format.data, ap);
    assert(length1 >= 0);

    str_buffer_ensure_capacity(buffer, length1);

    i32 length2 = vsnprintf(
        (char*) buffer->data + buffer->length, length1 + 1, (char*) format.data,
        ap2
    );
    assert(length1 == length2);

    va_end(ap2);

    buffer->length += length1;

    return length1;
}

// __attribute__((format(printf, 2, 3)))
static i32 str_buffer_append_printf(
    StrBuffer buffer[static 1], Str format, ...
) {
    va_list ap;
    va_start(ap, format);
    i32 length = str_buffer_append_vprintf(buffer, format, ap);
    va_end(ap);
    return length;
}
