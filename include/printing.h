#pragma once

#include "defs.h"

struct _ast_element;
struct type_context;
struct type;
struct instruction;

void print_element(struct _ast_element* element, usize indent);
void _print_type(struct type_context* context, struct type* type, usize indent);
void print_instruction(struct instruction instruction, usize indent);
