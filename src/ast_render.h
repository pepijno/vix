#pragma once

#include "parser.h"

#include <stdio.h>

void ast_print(FILE* f, AstNode* node, i64 indent);
