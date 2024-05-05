#include "generator.h"

static void
generate_program_begin() {
    fprintf(
        stdout,
        ""
        "section .data\n"
        "    ; Data section (if needed)\n"
        "\n"
        "section .text\n"
        "    global main\n"
        "\n"
    );
}

static void
generate_program_end() {
    fprintf(
        stdout,
        ""
        "main:\n"
        "    mov     rax, 60\n"
        "    mov     rbx, 0\n"
        "    syscall\n"
    );
}

void
generate(struct ast_object_t root[static 1]) {
    (void) root;
    generate_program_begin();

    generate_program_end();
}
