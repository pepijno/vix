#include "utf8.h"

#include <stdio.h>

static u8 const utf8_length[]
    = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 3, 4 };

#define utf8length(byte) utf8_length[(byte) >> 4]

struct utf8char
utf8_encode(struct codepoint const point) {
    if (!point.ok) {
        return UTF8_INVALID;
    }
    u32 cp = point.character;
    if (cp < 0x80) {
        return (struct utf8char){
            .characters = { cp },
            .length     = 1,
            .ok         = true,
        };
    } else if (cp < 0x800) {
        return (struct utf8char){
            .characters =
                {
                             0xC0 | (cp >> 6),
                             0x80 | (cp & 0x3F),
                             },
            .length = 2,
            .ok     = true,
        };
    } else if (cp < 0x10000) {
        return (struct utf8char){
            .characters =
                {
                             0xE0 | (cp >> 12),
                             0x80 | ((cp >> 6) & 0x3F),
                             0x80 | (cp & 0x3F),
                             },
            .length = 3,
            .ok     = true,
        };
    } else if (cp < 110000) {
        return (struct utf8char){
            .characters =
                {
                             0xF0 | (cp >> 18),
                             0x80 | ((cp >> 12) & 0x3F),
                             0x80 | ((cp >> 6) & 0x3F),
                             0x80 | (cp & 0x3F),
                             },
            .length = 4,
            .ok     = true,
        };
    } else {
        return UTF8_INVALID;
    }
}

struct codepoint
utf8_decode(struct utf8char const utf8_char) {
    struct codepoint cp = {
        .ok = true,
    };
    if (!utf8_char.ok) {
        return CODEPOINT_INVALID;
    }
    usize length;
    if ((utf8_char.characters[0] & 0x80) == 0x0) {
        cp.character = utf8_char.characters[0];
        length       = 1;
    } else if ((utf8_char.characters[0] & 0xE0) == 0xC0) {
        cp.character = utf8_char.characters[0] & 0x1F;
        length       = 2;
    } else if ((utf8_char.characters[0] & 0xF0) == 0xE0) {
        cp.character = utf8_char.characters[0] & 0x0F;
        length       = 3;
    } else if ((utf8_char.characters[0] & 0xF8) == 0xF0) {
        cp.character = utf8_char.characters[0] & 0x07;
        length       = 4;
    } else {
        return CODEPOINT_INVALID;
    }

    for (usize i = 1; i < length; i += 1) {
        if ((utf8_char.characters[i] & 0xC0) != 0x80) {
            return CODEPOINT_INVALID;
        }
        cp.character = (cp.character << 6) | (utf8_char.characters[i] & 0x3F);
    }

    return cp;
}

struct codepoint
utf8_get(FILE f[static const 1]) {
    if (feof(f)) {
        return CODEPOINT_INVALID;
    }
    struct utf8char utf8 = {
        .ok = true,
    };
    utf8.characters[0] = fgetc(f);
    u8 size            = utf8length(utf8.characters[0]);

    if (size > UTF8_MAX_SIZE || size == 0) {
        fseek(f, size - 1, SEEK_CUR);
        return CODEPOINT_INVALID;
    }

    for (i32 i = 1; i < size; i += 1) {
        if (feof(f)) {
            return CODEPOINT_INVALID;
        }
        utf8.characters[i] = fgetc(f);
    }
    return utf8_decode(utf8);
}
