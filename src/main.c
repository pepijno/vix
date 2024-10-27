#include "allocator.h"
#include "analyser.h"
#include "ast.h"
#include "emit.h"
#include "generator.h"
#include "lexer.h"
#include "qbe.h"

#include <stdio.h>
#include <stdlib.h>

i32
main(i32 argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    usize arena_buffer_size = 1024 * 1024 * 16;
    u8* arena_buffer        = malloc(arena_buffer_size);
    struct arena arena      = arena_init(arena_buffer, arena_buffer_size);

    FILE* f            = fopen(argv[1], "rb");
    struct lexer lexer = lexer_new(&arena, f, 0);
    sources            = arena_allocate(&arena, 2 * sizeof(struct string*));
    sources[0]         = from_cstr(argv[1]);

    struct ast_object* root = parse(&arena, &lexer);
    print_object(root, 0);

    // struct function_type* root_type = analyse(&arena, root);
    //
    // struct qbe_program program = {};
    // program.next               = &program.definitions;
    //
    // generate(&arena, &program, root, root_type);
    //
    // emit(&program, stdout);

    return 0;
}
