#include "generator.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    // int const arena_buffer_size = 1024 * 1024 * 16;
    // void* arena_buffer          = malloc(arena_buffer_size);
    // Arena arena                 = arena_init(arena_buffer, arena_buffer_size);
    // Allocator allocator         = init_arena_allocator(&arena);

    FILE* f    = fopen(argv[1], "rb");
    struct lexer_t lexer = lexer_new(f, 0);
    sources = calloc(2, sizeof(char**));
    sources[0] = argv[1];
    // while (true) {
    //     struct token_t token = {};
    //     enum lex_token_type_e type = lex(&lexer, &token);
    //     printf("%s\n", token_names[type]);
    //     if (type == TOKEN_EOF) {
    //         break;
    //     }
    // }
    // lexer_finish(&lexer);

    struct ast_object_t* root = parse(&lexer);
    // print_object(root, 0);

    generate(root);

    // TokenPtrArray tokens = array(Token*, &allocator);
    // Tokenized tokenized  = {};
    // tokenized.error      = str_new_empty();
    // tokenize(&allocator, source, &tokens, &tokenized);
    //
    // if (tokenized.error.length != 0) {
    //     ErrorMessage error_message = error_message_create_with_line(
    //         &allocator, path, tokenized.error_line, tokenized.error_column,
    //         source, tokenized.line_offsets, tokenized.error
    //     );
    //
    //     print_error_message(&error_message, ERROR_COLOR_ON);
    //     exit(1);
    // }
    //
    // print_tokens(source, *tokenized.tokens);

    // ImportTableEntry import_entry = {
    //     .source_code  = source,
    //     .line_offsets = tokenized.line_offsets,
    //     .path         = path,
    // };
    // import_entry.root = ast_parse(
    //     &allocator, source, tokenized.tokens, &import_entry, ERROR_COLOR_ON
    // );
    // assert(import_entry.root != nullptr);
    //
    // ast_print(stdout, import_entry.root, 0);
    //
    // Analyzer analyzer = {
    //     .error_color = ERROR_COLOR_ON,
    //     .allocator   = &allocator,
    //     .root_scope =
    //         {
    //                      .parent      = nullptr,
    //                      .source_node = import_entry.root,
    //                      },
    // };
    // analyzer.errors                     = array(ErrorMessage*, &allocator);
    // analyzer.root_scope.properties      = array(AstNode*, &allocator);
    // analyzer.root_scope.free_properties = array(AstNode*, &allocator);
    //
    // analyse(&analyzer, import_entry.root);
    //
    // CodeGen code_gen = {
    //     .allocator = &allocator,
    //     .root_scope =
    //         {
    //                      .parent      = nullptr,
    //                      .source_node = import_entry.root,
    //                      },
    // };
    // analyzer.root_scope.properties      = array(AstNode*, &allocator);
    // analyzer.root_scope.free_properties = array(AstNode*, &allocator);
    //
    // StrBuffer buffer = str_buffer_new(&allocator, 0);
    // generate(&code_gen, &buffer, import_entry.root);
    //
    // FILE* temp = fopen("vix.asm", "w");
    // if (temp) {
    //     fprintf(temp, str_fmt, str_args(buffer));
    //     fclose(temp);
    // }
    //
    // system("nasm -g -felf64 vix.asm");
    // system("ld -o vix vix.o");

    // arena_free(&arena);

    return 0;
}
