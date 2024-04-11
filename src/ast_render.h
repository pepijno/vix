#pragma once

#include "parser.h"

#include <stdio.h>

void ast_print(FILE* const f, struct ast_node_t* node, size_t const indent);
