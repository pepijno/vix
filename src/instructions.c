#include "instructions.h"

#include "ast.h"
#include "compilation_env.h"
#include "vector.h"

#include <stdio.h>

VECTOR_IMPL(struct instruction, instruction)

void
compile(
    struct allocator allocator[static const 1],
    struct compilation_env const compilation_env,
    struct ast_element const element,
    struct vector_instruction instructions[static const 1]
) {
    switch (element.type) {
        case AST_ELEMENT_TYPE_INTEGER:
            vector_instruction_add(
                instructions, ((struct instruction){
                                  .type     = INSTRUCTION_TYPE_PUSH_INT,
                                  .push_int = ((struct instruction_push_int){
                                      .value = element.integer.value,
                                  }),
                              })
            );
            break;
        case AST_ELEMENT_TYPE_STRING:
            vector_instruction_add(
                instructions,
                ((struct instruction){
                    .type     = INSTRUCTION_TYPE_PUSH_STR,
                    .push_str = ((struct instruction_push_str){
                        .value
                        = string_duplicate(allocator, element.string.value),
                    }),
                })
            );
            break;
        // case AST_ELEMENT_TYPE_ID:
        //     if (has_variable(compilation_env, element.id.id)) {
        //         vector_instruction_add(
        //             instructions,
        //             ((struct instruction){
        //                 .type = INSTRUCTION_TYPE_PUSH,
        //                 .push = ((struct instruction_push){
        //                     .offset
        //                     = get_offset(compilation_env, element.id.id),
        //                 }),
        //             })
        //         );
        //     } else {
        //         vector_instruction_add(
        //             instructions,
        //             ((struct instruction){
        //                 .type        = INSTRUCTION_TYPE_PUSH_GLOBAL,
        //                 .push_global = ((struct instruction_push_global){
        //                     .name = string_duplicate(allocator, element.id.id),
        //                 }),
        //             })
        //         );
        //     }
        //     break;
        case AST_ELEMENT_TYPE_PROPERTIES:
            usize size = 0;
            vector_foreach(element.properties, prop) {
                compile(allocator, compilation_env, *prop->value, instructions);
                size += 1;
            }
            vector_instruction_add(
                instructions, ((struct instruction){
                                  .type = INSTRUCTION_TYPE_PACK,
                                  .pack = ((struct instruction_pack){
                                      .size = size,
                                  }),
                              })
            );
            break;
    }
}

void
emit(struct vector_instruction const instructions) {
    vector_foreach(instructions, instruction) {
        switch (instruction.type) {
            case INSTRUCTION_TYPE_PUSH_INT:
                printf(
                    "\t%%node =l call $create_number_node(l %ld)\n",
                    instruction.push_int.value
                );
                printf("\t%%stack =l call $stack_push(l %%stack, l %%node)\n");
                break;
            case INSTRUCTION_TYPE_PUSH_STR:
                break;
            case INSTRUCTION_TYPE_PUSH_GLOBAL:
                break;
            case INSTRUCTION_TYPE_PUSH:
                break;
            case INSTRUCTION_TYPE_PACK:
                printf(
                    "\t%%stack =l call $stack_pack(l %%stack, l %ld, b %d)\n",
                    instruction.pack.size, instruction.pack.tag
                );
                break;
            case INSTRUCTION_TYPE_SPLIT:
                printf(
                    "\t%%stack =l call $stack_split(l %%stack, l %ld)\n",
                    instruction.split.size
                );
                break;
        }
    }
}
