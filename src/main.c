#include "allocator.h"
#include "analyser.h"
#include "array.h"
#include "ast_render.h"
#include "error_message.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static struct str_t os_fetch_file(
    struct allocator_t allocator[static const 1], FILE f[static const 1]
) {
    static size_t const buffer_size = 0x2000;
    struct str_buffer_t buffer      = str_buffer_new(allocator, buffer_size);
    size_t actual_buffer_length     = 0;
    while (true) {
        size_t amt_read = fread(
            buffer.data + actual_buffer_length, sizeof(char), buffer_size, f
        );
        actual_buffer_length += amt_read;
        buffer.length = actual_buffer_length;
        if (amt_read != buffer_size) {
            if (feof(f)) {
                return str_buffer_str(&buffer);
            } else {
                exit(1);
            }
        }

        str_buffer_ensure_capacity(&buffer, buffer_size);
    }
    vix_unreachable();
}

int main(int const argc, char const* const argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    struct arena_t arena         = {};
    struct allocator_t allocator = init_arena_allocator(&arena);

    struct str_t path   = str_new(argv[1]);
    FILE* f             = fopen(path.data, "rb");
    struct str_t source = os_fetch_file(&allocator, f);
    fclose(f);

    token_ptr_array_t tokens     = array(struct token_t*, &allocator);
    struct tokenized_t tokenized = {};
    tokenized.error              = str_new_empty();
    tokenize(&allocator, source, &tokens, &tokenized);

    if (tokenized.error.length != 0) {
        struct error_message_t const error_message =
            error_message_create_with_line(
                &allocator, path, tokenized.error_line, tokenized.error_column,
                source, tokenized.line_offsets, tokenized.error
            );

        print_error_message(&error_message, ERROR_COLOR_ON);
        exit(1);
    }

    print_tokens(source, *tokenized.tokens);

    struct import_table_entry_t import_entry = {
        .source_code  = source,
        .line_offsets = tokenized.line_offsets,
        .path         = path,
    };
    import_entry.root = ast_parse(
        &allocator, source, tokenized.tokens, &import_entry,
        ERROR_COLOR_ON
    );
    assert(import_entry.root != nullptr);

    ast_print(stdout, import_entry.root, 0);

    struct code_gen_t code_gen = {
        .error_color = ERROR_COLOR_ON,
        .allocator   = &allocator,
        .root_scope =
            {
                         .parent      = nullptr,
                         .source_node = import_entry.root,
                         },
    };
    code_gen.errors                = array(struct error_message_t*, &allocator);
    code_gen.root_scope.properties = array(struct ast_node_t*, &allocator);

    analyse(&code_gen, import_entry.root);
    struct str_t generated = generate(&code_gen, import_entry.root);

    printf(str_fmt"\n", str_args(generated));

    arena_reset(&arena);
    arena_free(&arena);

    return 0;
}
