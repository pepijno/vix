#include "analyser.h"
#include "emit.h"
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

    FILE* f            = fopen(argv[1], "rb");
    struct lexer lexer = lexer_new(f, 0);
    sources            = calloc(2, sizeof(char**));
    sources[0]         = argv[1];

    struct ast_object* root = parse(&lexer);
    print_object(root, 0);

    struct function_type* root_type = analyse(root);

    struct qbe_program program = {};
    program.next = &program.definitions;

    generate(&program, root, root_type);

    emit(&program, stdout);

    return 0;
}
