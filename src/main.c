#include "analyser.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

i32
main(i32 argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    // int const arena_buffer_size = 1024 * 1024 * 16;
    // void* arena_buffer          = malloc(arena_buffer_size);
    // Arena arena                 = arena_init(arena_buffer,
    // arena_buffer_size); Allocator allocator         =
    // init_arena_allocator(&arena);

    FILE* f              = fopen(argv[1], "rb");
    struct lexer lexer = lexer_new(f, 0);
    sources              = calloc(2, sizeof(char**));
    sources[0]           = argv[1];
    // while (true) {
    //     struct token_t token = {};
    //     enum lex_token_type_e type = lex(&lexer, &token);
    //     printf("%s\n", token_names[type]);
    //     if (type == TOKEN_EOF) {
    //         break;
    //     }
    // }
    // lexer_finish(&lexer);

    struct ast_object* root = parse(&lexer);
    print_object(root, 0);

    analyse(root);

    generate(root);

    return 0;
}
