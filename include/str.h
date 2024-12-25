#pragma once

#include "defs.h"
#include "hashset.h"

struct string {
    char* data;
    usize length;
};

HASHSET_DEFS(struct string, string)

struct string from_cstr(char* str);

bool strings_equal(struct string const s1, struct string const s2);

struct arena;

struct string string_init_empty(struct arena* arena, usize size);
struct string string_grow(
    struct arena* arena, struct string string, usize new_size
);
struct string string_duplicate(struct arena* arena, struct string const string);

u32 fnv1a_s(u32 const hash, struct string const string);
u32 string_hash(struct string const string);

#define STR_FMT      "%.*s"
#define STR_ARG(str) (int) (str).length, (str).data
