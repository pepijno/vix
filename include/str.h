#pragma once

#include "defs.h"

struct string {
    char* buffer;
    usize length;
};

struct string create_from_cstring(char* str);

bool strings_equal(struct string const s1, struct string const s2);

struct arena;

struct string string_duplicate(struct arena* arena, struct string const string);
