#include "generator.h"

static void
generate_object_data(struct ast_object object[static 1]) {
    object = object;
}

static void
generate_program_begin(struct ast_object root[static 1]) {
    root = root;
    fprintf(
        stdout,
        ""
        "section .data\n"
    );

    fprintf(
        stdout,
        ""
        "\n"
        "section .text\n"
        "    global _start\n"
        "\n"
    );
}

static void
generate_program_end() {
    fprintf(
        stdout,
        ""
        "_start:\n"
        "    mov rdi, 0\n"
        "    mov rax, 60\n"
        "    syscall\n"
    );
}

void
generate(struct ast_object root[static 1]) {
    (void) root;
    generate_program_begin(root);

    generate_object_data(root);

    generate_program_end();
}
