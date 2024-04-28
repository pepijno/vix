#include "generator.h"

#include "array.h"
#include "util.h"

char const* registers[3] = {"rdi", "rsi", "rdx"};

void generate_free_pop(
    StrBuffer buffer[static 1], AstNode node[static 1], char const* reg
) {
    printf("%ld\n", node->id);
    switch (node->type) {
        case NODE_TYPE_STRING_LITERAL:
        case NODE_TYPE_CHAR_LITERAL:
        case NODE_TYPE_INTEGER:
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "    mov %s, " str_fmt "\n"),
                reg, str_args(node->integer.content)
            );
            break;
        case NODE_TYPE_OBJECT:
        case NODE_TYPE_PROPERTY:
        case NODE_TYPE_OBJECT_COPY:
            break;
        default:
            vix_unreachable();
    }
}

static AstNode* findNode(AstNode node[static 1], Str id, bool only_children) {
    if (node->type != NODE_TYPE_ROOT && node->type != NODE_TYPE_OBJECT) {
        if (only_children) {
            vix_unreachable();
        } else {
            return findNode(node->parent, id, only_children);
        }
    }
    if (node->type == NODE_TYPE_ROOT) {
        for (i64 i = 0; i < array_length(node->root.list); ++i) {
            AstNode* n = node->root.list[i];
            if (n->type != NODE_TYPE_PROPERTY) {
                continue;
            }
            if (str_equal(id, n->property.identifier->identifier.content)) {
                return n;
            }
        }
        vix_unreachable();
    } else if (node->type == NODE_TYPE_OBJECT) {
        for (i64 i = 0; i < array_length(node->object.free_list); ++i) {
            AstNode* n = node->object.free_list[i];
            if (n->type != NODE_TYPE_IDENTIFIER) {
                continue;
            }
            if (str_equal(id, n->identifier.content)) {
                return n;
            }
        }
        for (i64 i = 0; i < array_length(node->object.property_list); ++i) {
            AstNode* n = node->object.property_list[i];
            if (n->type != NODE_TYPE_PROPERTY) {
                continue;
            }
            if (str_equal(id, n->property.identifier->identifier.content)) {
                return n;
            }
        }
        if (only_children) {
            vix_unreachable();
        } else {
            return findNode(node->parent, id, only_children);
        }
    }
    return nullptr;
}

static void generate_node(
    StrBuffer buffer[static 1], CodeGen code_gen[static 1],
    AstNode node[static 1], AstNode** free_properties
) {
    switch (node->type) {
        case NODE_TYPE_ROOT: {
            // str_buffer_append_printf(
            //     buffer, str_new(""
            //                     "_root:\n"
            //                     "    push    rbp\n"
            //                     "    mov     rbp, rsp\n"
            //                     "    mov     QWORD [rbp-4], rdi\n")
            // );
            // for (i64 i = 0; i < array_length(node->root.list); ++i) {
            //     AstNode* n = node->root.list[i];
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    cmp     QWORD [rbp-4], %ld\n"
            //                 "    je      _root_%ld\n"),
            //         n->id, n->id
            //     );
            // }
            // str_buffer_append_printf(
            //     buffer, str_new(""
            //                     "    jmp     _root_end\n")
            // );
            // for (i64 i = 0; i < array_length(node->root.list); ++i) {
            //     AstNode* n = node->root.list[i];
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "_root_%ld:             ;" str_fmt "\n"
            //                 "    mov     rax, _%ld\n"
            //                 "    jmp     _root_end\n"),
            //         n->id, str_args(n->property.identifier->identifier.content),
            //         n->id
            //     );
            // }
            // str_buffer_append_printf(
            //     buffer, str_new(""
            //                     "_root_end:\n"
            //                     "    nop\n"
            //                     "    leave\n"
            //                     "    ret\n"
            //                     "\n")
            // );
            for (i64 i = 0; i < array_length(node->root.list); ++i) {
                AstNode* n = node->root.list[i];
                generate_node(buffer, code_gen, n, free_properties);
            }
            break;
        }
        case NODE_TYPE_OBJECT: {
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "_%ld:\n"
                        "    push    rbp\n"
                        "    mov     rbp, rsp\n"
                        "    mov     QWORD [rbp-4], rdi\n"),
                node->parent->id
            );
            AstNode** new_free_properties =
                array(AstNode*, code_gen->allocator);
            for (i64 i = 0; i < array_length(free_properties); ++i) {
                array_push(new_free_properties, free_properties[i]);
            }
            for (i64 i = 0; i < array_length(node->object.free_list); ++i) {
                array_push(new_free_properties, node->object.free_list[i]);
            }
            for (i64 i = 0; i < array_length(new_free_properties); ++i) {
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "    ; " str_fmt "\n"),
                    str_args(new_free_properties[i]->identifier.content)
                );
            }
            for (i64 i = 0; i < array_length(node->object.property_list); ++i) {
                AstNode* n = node->object.property_list[i];
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "    cmp     QWORD [rbp-4], %ld\n"
                            "    je      _%ld_%ld\n"),
                    n->id, node->parent->id, n->id
                );
            }
            for (i64 i = 0; i < array_length(new_free_properties); ++i) {
                AstNode* n = new_free_properties[i];
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "    cmp     QWORD [rbp-4], %s\n"
                            "    je      _%ld_%ld\n"),
                    registers[i + 1], node->parent->id, n->id
                );
            }
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "    jmp     _%ld_end\n"),
                node->parent->id
            );
            for (i64 i = 0; i < array_length(node->object.property_list); ++i) {
                AstNode* n = node->object.property_list[i];
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "_%ld_%ld:                ;" str_fmt "\n"
                            "    mov     rax, _%ld\n"
                            "    jmp     _%ld_end\n"),
                    node->parent->id, n->id,
                    str_args(n->property.identifier->identifier.content), n->id,
                    node->parent->id
                );
            }
            for (i64 i = 0; i < array_length(new_free_properties); ++i) {
                AstNode* n = new_free_properties[i];
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "_%ld_%ld:                ;" str_fmt "\n"
                            "    mov     rax, %s\n"
                            "    jmp     _%ld_end\n"),
                    node->parent->id, n->id, str_args(n->identifier.content),
                    registers[i + 1], node->parent->id
                );
            }
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "_%ld_end:\n"
                        "    nop\n"
                        "    leave\n"
                        "    ret\n"
                        "\n"),
                node->parent->id
            );
            for (i64 i = 0; i < array_length(node->object.property_list); ++i) {
                AstNode* n = node->object.property_list[i];
                generate_node(buffer, code_gen, n, new_free_properties);
            }
            break;
        }
        case NODE_TYPE_PROPERTY: {
            generate_node(
                buffer, code_gen, node->property.property_value, free_properties
            );
            break;
        }
        case NODE_TYPE_STRING_LITERAL:
            break;
        case NODE_TYPE_CHAR_LITERAL:
            break;
        case NODE_TYPE_INTEGER:
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "_%ld:\n"
                        "    mov rax, " str_fmt "\n"
                        "    ret\n"
                        "\n"),
                node->parent->id, str_args(node->integer.content)
            );
            break;
        case NODE_TYPE_IDENTIFIER:
            break;
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
            break;
        case NODE_TYPE_OBJECT_COPY: {
            // str_buffer_append_printf(
            //     buffer,
            //     str_new(""
            //             "_%ld:\n"),
            //     node->parent->id
            // );
            // for (i64 i = 0; i < array_length(free_properties); ++i) {
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    ; " str_fmt "\n"),
            //         str_args(free_properties[i]->identifier.content)
            //     );
            // }
            // if (array_length(node->object_copy.free_object_copies) > 0) {
            //     for (i64 i = 0;
            //          i <
            //          array_length(node->object_copy.free_object_copies[0]
            //                           ->free_object_copy_params.parameter_list);
            //          ++i) {
            //         str_buffer_append_printf(
            //             buffer, str_new(""
            //                             "    push rax\n")
            //         );
            //         generate_free_pop(
            //             buffer,
            //             node->object_copy.free_object_copies[0]
            //                 ->free_object_copy_params.parameter_list[i],
            //             registers[i + 1]
            //         );
            //         str_buffer_append_printf(
            //             buffer, str_new(""
            //                             "    pop rax\n")
            //         );
            //     }
            // }
            // AstNode* n = findNode(
            //     node->parent, node->object_copy.identifier->identifier.content,
            //     false
            // );
            // bool is_in_free_list = false;
            // int free_list_id     = 0;
            // for (i64 i = 0; i < array_length(free_properties); ++i) {
            //     if (str_equal(
            //             node->object_copy.identifier->identifier.content,
            //             free_properties[i]->identifier.content
            //         )) {
            //         is_in_free_list = true;
            //         free_list_id    = i + 1;
            //         break;
            //     }
            // }
            // if (is_in_free_list) {
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    mov rdi, %s\n"),
            //         registers[free_list_id]
            //     );
            // } else if (n->parent->type == NODE_TYPE_ROOT) {
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    call _%ld\n"),
            //         n->id
            //     );
            // } else {
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    mov rdi, %ld\n"
            //                 "    call _%ld\n"),
            //         n->id, n->id
            //     );
            // }
            // AstNode* child = node->object_copy.object_copy;
            // while (child != nullptr) {
            //     if (array_length(child->object_copy.free_object_copies) > 0) {
            //         for (i64 i = 0;
            //              i < array_length(
            //                      child->object_copy.free_object_copies[0]
            //                          ->free_object_copy_params.parameter_list
            //                  );
            //              ++i) {
            //             str_buffer_append_printf(
            //                 buffer, str_new(""
            //                                 "    push rax\n")
            //             );
            //             generate_free_pop(
            //                 buffer,
            //                 child->object_copy.free_object_copies[0]
            //                     ->free_object_copy_params.parameter_list[i],
            //                 registers[i + 1]
            //             );
            //             str_buffer_append_printf(
            //                 buffer, str_new(""
            //                                 "    pop rax\n")
            //             );
            //         }
            //     }
            //     n = findNode(
            //         n->property.property_value,
            //         child->object_copy.identifier->identifier.content, true
            //     );
            //     str_buffer_append_printf(
            //         buffer,
            //         str_new(""
            //                 "    mov rdi, %ld\n"
            //                 "    call rax\n"),
            //         n->id
            //     );
            //     child = child->object_copy.object_copy;
            // }
            // str_buffer_append_printf(
            //     buffer, str_new(""
            //                     "    ret\n"
            //                     "\n")
            // );
            str_buffer_append_printf(
                buffer,
                str_new(""
                        "_%ld:\n"),
                node->parent->id
            );
            for (i64 i = 0; i < array_length(free_properties); ++i) {
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "    ; " str_fmt "\n"),
                    str_args(free_properties[i]->identifier.content)
                );
            }
            AstNode* n = findNode(
                node->parent, node->object_copy.identifier->identifier.content,
                false
            );
            AstNode* child = node->object_copy.object_copy;
            bool is_in_free_list = false;
            int free_list_id     = 0;
            for (i64 i = 0; i < array_length(free_properties); ++i) {
                if (str_equal(
                        node->object_copy.identifier->identifier.content,
                        free_properties[i]->identifier.content
                    )) {
                    is_in_free_list = true;
                    free_list_id    = i + 1;
                    break;
                }
            }
            if (is_in_free_list) {
                str_buffer_append_printf(
                    buffer,
                    str_new(""
                            "    call %s\n"),
                    registers[free_list_id]
                );
            } else if (child == nullptr) {
                str_buffer_append_printf(
                    buffer, str_new(""
                                    "    call _%ld\n"
                                    ),
                    n->id
                );
            } else {
                AstNode* n2 = findNode(
                    n->property.property_value,
                    child->object_copy.identifier->identifier.content, true
                );
                str_buffer_append_printf(
                    buffer, str_new(""
                                    "    mov rdi, %ld\n"
                                    "    call _%ld\n"
                                    ),
                    n2->id,
                    n->id
                );
                child = child->object_copy.object_copy;
                n = n2;
                while (child != nullptr) {
                    n = findNode(
                        n->property.property_value,
                        child->object_copy.identifier->identifier.content, true
                    );
                    str_buffer_append_printf(
                        buffer,
                        str_new(""
                                "    mov rdi, %ld\n"
                                "    call rax\n"),
                        n->id
                    );
                    child = child->object_copy.object_copy;
                }
                str_buffer_append_printf(
                    buffer, str_new(""
                                    "    call rax\n"
                                    )
                );
            }
            str_buffer_append_printf(
                buffer, str_new(""
                                "    ret\n"
                                "\n")
            );
            break;
        }
        case NODE_TYPE_DECORATOR:
            break;
    }
}

static void write_program_start(
    StrBuffer buffer[static 1], AstNode root[static 1]
) {
    str_buffer_append_printf(
        buffer,
        str_new(
            ""
            // "section .data\n"
            // "    error_msg db \"Error: Memory allocation failed\", 0\n"
            // "create_array:\n"
            // "    ; Input: \n"
            // "    ;   rdi = number of elements (n)\n"
            // "    ; Output: \n"
            // "    ;   rax = address of the allocated memory (array)\n"
            // "\n"
            // "    ; Calculate the total size of the array in bytes\n"
            // "    ; Example: Size of each element is 4 bytes (assuming 32-bit "
            // "integers)\n"
            // "    mov rcx, rdi        ; rcx = n\n"
            // "    mov rdx, 4          ; Size of each element in bytes\n"
            // "    imul rcx, rdx       ; rcx = n * 4 (total size in bytes)\n"
            // "\n"
            // "    ; Call brk to allocate memory\n"
            // "    mov rbx, rcx        ; rbx = size of memory to allocate\n"
            // "    mov rax, 45         ; brk syscall number\n"
            // "    int 0x80            ; Call syscall\n"
            // "\n"
            // "    ; Check for errors in brk syscall\n"
            // "    test rax, rax       ; Test if rax contains an error code\n"
            // "    jns allocated       ; Jump if no error (sign flag not set)\n"
            // "\n"
            // "    ; Handle error (rax contains error code)\n"
            // "    ; Example: Print error message and exit\n"
            // "    mov rax, 4          ; sys_write syscall number\n"
            // "    mov rbx, 2          ; file descriptor 2 (stderr)\n"
            // "    mov rcx, error_msg  ; address of error message\n"
            // "    jmp exit_program\n"
            // "\n"
            // "allocated:\n"
            // "    ; rbx contains the address of the allocated memory\n"
            // "    mov rax, rbx        ; Return the address of the allocated "
            // "memory in rax\n"
            // "    ret\n"
            // "exit_program:\n"
            // "    mov rax, 1          ; Exit syscall number\n"
            // "    xor rbx, rbx        ; Exit status: 1\n"
            // "    int 0x80            ; Perform syscall\n"
            // "\n"
            ".text\n"
            "    global _start\n"
        )
    );
}

static void write_program_end(
    StrBuffer buffer[static 1], CodeGen code_gen[static 1],
    AstNode root[static 1]
) {
    i64 id = 0;
    for (i64 i = 0; i < array_length(root->root.list); ++i) {
        AstNode* n = root->root.list[i];
        if (str_equal(
                n->property.identifier->identifier.content, str_new("main")
            )) {
            id = n->id;
            break;
        }
    }
    str_buffer_append_printf(
        buffer,
        str_new(""
                "_start:\n"
                "    mov rax, 0\n"
                "    call _%ld\n"
                "    mov rdi, rax\n"
                "    mov rax, 60\n"
                "    syscall\n"),
        id
    );
}

void generate(
    CodeGen code_gen[static 1], StrBuffer buffer[static 1],
    AstNode root[static 1]
) {
    write_program_start(buffer, root);

    AstNode** free_properties = array(AstNode*, code_gen->allocator);
    generate_node(buffer, code_gen, root, free_properties);

    write_program_end(buffer, code_gen, root);
}
