#include "allocator.h"
#include "ast.h"
#include "compilation_env.h"
#include "defs.h"
#include "graph.h"
#include "hash.h"
#include "hashmap.h"
#include "instructions.h"
#include "lexer.h"
#include "parser.h"
#include "printing.h"
#include "types.h"
#include "vector.h"

#include <stdio.h>
#include <stdlib.h>

u64
hash(u8* bytes) {
    return fnv1a_u64(HASH_INIT, *((u64*) bytes));
}

bool
keys_equal(u8* key1, u8* key2) {
    return *((u64*) key1) == *((u64*) key2);
}

bool
is_key_zero(u8* bytes) {
    return *((u64*) bytes) == 0;
}

i32
main(i32 argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    usize arena_buffer_size = 1024 * 1024 * 128;
    u8* arena_buffer        = malloc(arena_buffer_size);
    struct arena arena      = arena_init(arena_buffer, arena_buffer_size);

    FILE* f            = fopen(argv[1], "rb");
    struct lexer lexer = lexer_new(&arena, f, 0);
    sources            = arena_allocate(&arena, 2 * sizeof(struct string*));
    sources[0]         = from_cstr(argv[1]);

    struct parser_context parser_context = {
        .arena      = &arena,
        .next_id    = 1,
        .properties = hashmap_properties_new(&arena),
    };

    struct _ast_element* root = _parse(&parser_context, &lexer);
    print_element(root, 0);

    struct object_graph obj_graph = {
        .edges           = hashset_edge_new(&arena),
        .adjacency_lists = hashmap_adjacency_new(&arena),
    };

    struct typecheck_env typecheck_env = {
        .parent = nullptr,
        .names  = hashmap_string_id_new(&arena),
    };

    typecheck_init_properties(
        &arena, &obj_graph, root->properties, &typecheck_env
    );

    auto groups = graph_compute_order(&arena, &obj_graph);

    vector_foreach(groups, group) {
        hashset_foreach(group->members, member) {
            auto str
                = hashmap_properties_find(&parser_context.properties, member);
            printf(STR_FMT ", ", STR_ARG(str->name));
        }
        printf("\n");
    }

    struct type_context context = {
        .last_id = 1,
        .arena   = &arena,
        .types   = hashmap_string_type_new(&arena),
    };
    struct type_env env = {
        .arena = &arena,
        .names = hashmap_string_type_new(&arena),
    };

    vector_foreach(groups, group) {
        hashset_foreach(group->members, member) {
            auto prop
                = hashmap_properties_find(&parser_context.properties, member);
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
        _print_type(&context, type.value, 0);
    }

    struct compilation_env compilation_env = {
        .type       = COMPILATION_ENV_TYPE_OFFSET,
        .parent     = nullptr,
        .env_offset = ((struct compilation_env_offset){
            .offset = 0,
        }),
    };
    vector_foreach(root->properties, prop) {
        auto instructions = vector_instruction_new(&arena);
        compile(&arena, compilation_env, *prop->value, &instructions);
        vector_foreach(instructions, instruction) {
            print_instruction(instruction, 0);
        }
        _emit(instructions);
    }

    return 0;
}
