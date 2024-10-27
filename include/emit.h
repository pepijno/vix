#pragma once

#include <stdio.h>

struct qbe_program;

void emit(struct qbe_program* program, FILE* out);
