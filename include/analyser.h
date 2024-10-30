#pragma once

struct arena;
struct ast_object;
struct scope;

struct context {
    struct arena* arena;
    struct scope* scope;
};

void analyse(struct context* context, struct ast_object* root);
