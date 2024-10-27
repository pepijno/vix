#pragma once

#include "defs.h"

#include <execinfo.h>
#include <stdnoreturn.h>
#include <unistd.h>

extern char const** sources;

enum exit_status {
    EXIT_USER     = 1,
    EXIT_LEX      = 2,
    EXIT_PARSE    = 3,
    EXIT_VALIDATE = 4,
    EXIT_ABNORMAL = 255
};

// __attribute__((format(printf, 1, 2)))
noreturn void vix_panic(char* format, ...);

noreturn void vix_unreachable(void);

struct location {
    i32 file;
    i32 line_number;
    i32 column_number;
};

struct arena;

void error_line(struct arena* arena, struct location location);
