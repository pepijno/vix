#pragma once

#include "defs.h"

#include <stdbool.h>
#include <stdio.h>

#define UTF8_MAX_SIZE 4
#define CODEPOINT_INVALID \
    (struct codepoint) {  \
        .ok = false       \
    }
#define UTF8_INVALID    \
    (struct utf8char) { \
        .ok = false     \
    }

struct __attribute__((packed)) utf8char {
    u8 characters[UTF8_MAX_SIZE];
    u8 length;
    bool ok;
};
struct codepoint {
    u32 character;
    bool ok;
};

struct utf8char utf8_encode(struct codepoint point);
struct codepoint utf8_decode(struct utf8char utf8_char);
struct codepoint utf8_get(FILE* f);
