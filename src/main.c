#include "allocator.h"
#include "arena.h"
#include "ast.h"
#include "compilation_env.h"
#include "defs.h"
#include "graph.h"
#include "hashmap.h"
#include "instructions.h"
#include "lexer.h"
#include "parser.h"
#include "printing.h"
#include "types.h"
#include "vector.h"

#include <stdio.h>
#include <stdlib.h>

i32
main(i32 argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    usize arena_buffer_size    = 1024 * 1024 * 128;
    u8* arena_buffer           = malloc(arena_buffer_size);
    struct arena arena         = arena_init(arena_buffer_size, arena_buffer);
    struct allocator allocator = arena_allocator_init(&arena);

    FILE* f            = fopen(argv[1], "rb");
    struct lexer lexer = lexer_new(&allocator, f, 0);
    sources    = allocator_allocate(&allocator, 2 * sizeof(struct string*));
    sources[0] = from_cstr(argv[1]);

    struct parser_context parser_context = {
        .allocator  = &allocator,
        .next_id    = 1,
        .properties = hashmap_properties_new(&allocator),
    };

    struct ast_element* root = parse(&parser_context, &lexer);
    print_element(*root, 0);

    struct object_graph obj_graph = {
        .edges           = hashset_edge_new(&allocator),
        .adjacency_lists = hashmap_adjacency_new(&allocator),
    };

    struct typecheck_env typecheck_env = {
        .parent = nullptr,
        .names  = hashmap_string_id_new(&allocator),
    };

    typecheck_init_properties(
        &allocator, &obj_graph, root->properties, &typecheck_env
    );

    auto groups = graph_compute_order(&allocator, &obj_graph);

    vector_foreach(groups, group) {
        hashset_foreach(group->members, member) {
            auto search_result
                = hashmap_properties_find(parser_context.properties, member);
            assert(search_result.found);
            printf(STR_FMT ", ", STR_ARG(search_result.value->name));
        }
        printf("\n");
    }

    struct type_context context = {
        .last_id   = 1,
        .allocator = &allocator,
        .types     = hashmap_string_type_new(&allocator),
    };
    struct type_env env = {
        .allocator = &allocator,
        .names     = hashmap_string_type_new(&allocator),
    };

    vector_foreach(groups, group) {
        hashset_foreach(group->members, member) {
            auto search_result
                = hashmap_properties_find(parser_context.properties, member);
            assert(search_result.found);
            auto prop = search_result.value;
            if (prop->value->type == AST_ELEMENT_TYPE_PROPERTIES) {
                typecheck_properties_first(
                    &context, &env, prop->name, prop->value
                );
                typecheck_properties_second(&context, &env, prop->value);
            } else {
                auto type = typecheck(&context, &env, prop->value);
                env_bind(&env, prop->name, type);
            }
        }
    }

    hashmap_foreach(env.names, type) {
        printf(STR_FMT ": ", STR_ARG(type.key));
        print_type(context, type.value, 0);
    }

    struct compilation_env compilation_env = {
        .type       = COMPILATION_ENV_TYPE_OFFSET,
        .parent     = nullptr,
        .env_offset = ((struct compilation_env_offset){
            .offset = 0,
        }),
    };
    vector_foreach(root->properties, prop) {
        auto instructions = vector_instruction_new(&allocator);
        compile(&allocator, compilation_env, *prop->value, &instructions);
        vector_foreach(instructions, instruction) {
            print_instruction(instruction, 0);
        }
        emit(instructions);
    }

    return 0;
}
