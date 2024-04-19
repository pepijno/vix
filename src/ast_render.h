#pragma once

#include "parser.h"

#include <stdio.h>

void ast_print(FILE* f, ast_node_t* node, size_t indent);
