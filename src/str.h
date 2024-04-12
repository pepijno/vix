#pragma once

#include "allocator.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct str_header_t {
    size_t length;
    size_t capacity;
    struct allocator_t* allocator;
    char buffer[];
};

static inline size_t str_capacity(char const* const str) {
    return ((struct str_header_t*)str - 1)->length;
}

static inline size_t str_length(char const* const str) {
    return ((struct str_header_t*)str - 1)->length;
}

static inline size_t str_available(char const* const str) {
    return str_capacity(str) - str_length(str);
}

static inline struct allocator_t* str_allocator(char const* const str) {
    return ((struct str_header_t*)str - 1)->allocator;
}

static inline void str_set_length(char* const str, size_t const new_length) {
    ((struct str_header_t*)str - 1)->length = new_length;
}

static char* sstr_new_length(struct allocator_t* allocator, char const* const data, size_t const length) {
    size_t const header_size = sizeof(struct str_header_t);
    void* const ptr = allocator->allocate(header_size + length + 1, allocator->context);
    if (!ptr) {
        return nullptr;
    }
    if (!data) {
        memset(ptr, 0, header_size + length + 1);
    }
    char* str = (char*)ptr + header_size;
    ((struct str_header_t*)ptr)->length = length;
    ((struct str_header_t*)ptr)->capacity = length;
    ((struct str_header_t*)ptr)->allocator = allocator;
    if (length && data) {
        memcpy(str, data, length);
    }
    str[length] = '\0';
    return str;
}

static char* sstr_new_empty(struct allocator_t* allocator) {
    return sstr_new_length(allocator, "", 0);
}

static char* sstr_new(struct allocator_t* allocator, char const* const data) {
    size_t const length = (data) ? strlen(data) : 0;
    return sstr_new_length(allocator, data, length);
}

static char* sstr_copy(char const* const str) {
    return sstr_new_length(str_allocator(str), str, str_length(str));
}

static void str_free(char const* const str) {
    if (!str) {
        return;
    }
    struct allocator_t* allocator = str_allocator(str);
    allocator->free(((struct str_header_t*)str - 1), sizeof(struct str_header_t) + str_length(str) + 1, allocator->context);
}

static char* str_ensure_capacity(char* str, size_t const add_length) {
    size_t const available = str_available(str);
    if (available >= add_length) {
        return str;
    }
    size_t const length = str_length(str);
    void* header_ptr = (char*)str - sizeof(struct str_header_t);
    size_t const required_length = length + add_length;
    size_t new_length = 2 * required_length;
    struct allocator_t* allocator = str_allocator(str);
    void* new_header_ptr = allocator->reallocate(header_ptr, sizeof(struct str_header_t) + length + 1, sizeof(struct str_header_t) + new_length + 1, allocator->context);
    if (!new_header_ptr) {
        return NULL;
    }
    ((struct str_header_t*)new_header_ptr)->capacity = new_length;
    return (char*)new_header_ptr + sizeof(struct str_header_t);
}

static char* str_append_char(char* str, char const c) {
    char* new_str = str_ensure_capacity(str, 1);
    new_str[str_length(new_str)] = c;
    ((struct str_header_t*)new_str - 1)->length += 1;
    new_str[str_length(new_str)] = '\0';
    return new_str;
}

static char* str_cat(char* str1, char const* const str2) {
    char* new_str = str_ensure_capacity(str1, str_length(str2));
    size_t const length = str_length(new_str) == 0 ? 0 : str_length(new_str);
    memcpy(new_str + length, str2, str_length(str2));
    str_set_length(new_str, length + str_length(str2));
    return new_str;
}

static bool sstr_equal(char const* const str1, char const* const str2) {
    if (str_length(str1) != str_length(str2)) {
        return false;
    }
    if (str_length(str1) == 0 || str1 == str2) {
        return true;
    }
    for (size_t i = 0; i < str_length(str1); ++i) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

static char* str_vprintf(struct allocator_t allocator[static const 1], char const* const format, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);

    int length1 = vsnprintf(nullptr, 0, format, ap);
    assert(length1 >= 0);

    size_t const required_length = length1 + 1;

    char* str = sstr_new_length(allocator, nullptr, required_length);

    int length2 = vsnprintf(str, required_length, format, ap2);
    assert(length1 == length2);

    va_end(ap2);

    return str;
}

static char* str_printf(struct allocator_t allocator[static const 1], char const* const format, ...) {
    va_list ap;
    va_start(ap, format);
    char* str= str_vprintf(allocator, format, ap);
    va_end(ap);
    return str;
}
