#include "allocator.h"
#include "defs.h"

#include <string.h>

struct string
from_cstr(char* str) {
    return (struct string){
        .length = strlen(str),
        .data   = str,
    };
}

bool
strings_equal(struct string const s1, struct string const s2) {
    if (s1.length != s2.length) {
        return false;
    }
    for (usize i = 0; i < s1.length; i += 1) {
        if (s1.data[i] != s2.data[i]) {
            return false;
        }
    }
    return true;
}

struct string
string_init_empty(struct arena* arena, usize size) {
    return (struct string){
        .data = arena_allocate(arena, size * sizeof(char)),
        .length = size,
    };
}

struct string
string_grow(struct arena* arena, struct string string, usize new_size) {
    struct string new_string = string_init_empty(arena, new_size);
    memcpy(new_string.data, string.data, string.length);
    return new_string;
}

struct string
string_duplicate(struct arena* arena, struct string const string) {
    char* buffer = arena_allocate(arena, string.length);
    memcpy(buffer, string.data, string.length);
    return (struct string){
        .data   = buffer,
        .length = string.length,
    };
}
