#pragma once

#include "defs.h"

#include <stdbool.h>
#include <stdio.h>

#define UTF8_MAX_SIZE 4
#define CODEPOINT_INVALID  \
    (struct codepoint_t) { \
        .ok = false        \
    }
#define UTF8_INVALID      \
    (struct utf8char_t) { \
        .ok = false       \
    }

struct __attribute__((packed)) utf8char_t {
    u8 characters[UTF8_MAX_SIZE];
    u8 length;
    bool ok;
};
struct codepoint_t {
    u32 character;
    bool ok;
};

struct utf8char_t utf8_encode(struct codepoint_t point);
struct codepoint_t utf8_decode(struct utf8char_t utf8_char);
struct codepoint_t utf8_get(FILE* f);
