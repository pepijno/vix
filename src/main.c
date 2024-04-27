#include "allocator.h"
#include "analyser.h"
#include "array.h"
#include "ast_render.h"
#include "error_message.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static Str os_fetch_file(Allocator allocator[static 1], FILE f[static 1]) {
    static size_t buffer_size   = 0x2000;
    StrBuffer buffer            = str_buffer_new(allocator, buffer_size);
    size_t actual_buffer_length = 0;
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    int const arena_buffer_size = 1024 * 1024 * 16;
    void* arena_buffer          = malloc(arena_buffer_size);
    Arena arena                 = arena_init(arena_buffer, arena_buffer_size);
    Allocator allocator         = init_arena_allocator(&arena);

    Str path   = str_new(argv[1]);
    FILE* f    = fopen((char*) path.data, "rb");
    Str source = os_fetch_file(&allocator, f);
    fclose(f);

    TokenPtrArray tokens = array(Token*, &allocator);
    Tokenized tokenized  = {};
    tokenized.error      = str_new_empty();
    tokenize(&allocator, source, &tokens, &tokenized);

    if (tokenized.error.length != 0) {
        ErrorMessage error_message = error_message_create_with_line(
            &allocator, path, tokenized.error_line, tokenized.error_column,
            source, tokenized.line_offsets, tokenized.error
        );

        print_error_message(&error_message, ERROR_COLOR_ON);
        exit(1);
    }

    print_tokens(source, *tokenized.tokens);

    ImportTableEntry import_entry = {
        .source_code  = source,
        .line_offsets = tokenized.line_offsets,
        .path         = path,
    };
    import_entry.root = ast_parse(
        &allocator, source, tokenized.tokens, &import_entry, ERROR_COLOR_ON
    );
    assert(import_entry.root != nullptr);

    ast_print(stdout, import_entry.root, 0);

    Analyzer analyzer = {
        .error_color = ERROR_COLOR_ON,
        .allocator   = &allocator,
        .root_scope =
            {
                         .parent      = nullptr,
                         .source_node = import_entry.root,
                         },
    };
    analyzer.errors                = array(ErrorMessage*, &allocator);
    analyzer.root_scope.properties = array(AstNode*, &allocator);

    analyse(&analyzer, import_entry.root);

    CodeGen code_gen = {
        .allocator = &allocator,
        .root_scope =
            {
                         .parent      = nullptr,
                         .source_node = import_entry.root,
                         },
    };
    code_gen.data_strings = array(StringData, &allocator);
    StrBuffer buffer      = str_buffer_new(&allocator, 0);
    generate(&code_gen, &buffer, import_entry.root);

    FILE* temp = fopen("vix.c", "w");
    if (temp) {
        fprintf(temp, str_fmt, str_args(buffer));
        fclose(temp);
    }

    system("gcc -O2 -Wall -Wextra -o vix vix.c");

    arena_free(&arena);

    return 0;
}
