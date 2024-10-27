#include "str.h"

#include "allocator.h"

#include <string.h>

struct string create_from_cstring(char* str) {
    return (struct string){
        .length = strlen(str),
        .buffer = str,
    };
}

bool
strings_equal(struct string const s1, struct string const s2) {
    if (s1.length != s2.length) {
        return false;
    }
    for (usize i = 0; i < s1.length; i += 1) {
        if (s1.buffer[i] != s2.buffer[i]) {
            return false;
        }
    }
    return true;
}

struct string
string_duplicate(struct arena* arena, struct string const string) {
    char* buffer = arena_allocate(arena, string.length);
    memcpy(buffer, string.buffer, string.length);
    return (struct string){
        .buffer = buffer,
        .length = string.length,
    };
}
