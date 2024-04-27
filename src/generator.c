#include "generator.h"

static void generate_node(
    StrBuffer buffer[static 1], CodeGen code_gen[static 1],
    AstNode root[static 1]
) {
    switch (root->type) {
        case NODE_TYPE_ROOT:
            break;
        case NODE_TYPE_STRING_LITERAL:
        case NODE_TYPE_CHAR_LITERAL:
        case NODE_TYPE_INTEGER:
        case NODE_TYPE_OBJECT:
        case NODE_TYPE_PROPERTY:
        case NODE_TYPE_IDENTIFIER:
        case NODE_TYPE_FREE_OBJECT_COPY_PARAMS:
        case NODE_TYPE_OBJECT_COPY:
        case NODE_TYPE_DECORATOR:
            break;
    }
}

static void write_program_start(
    StrBuffer buffer[static 1], AstNode root[static 1]
) {
    str_buffer_printf(
        buffer, str_new(""
                        "#include <stdio.h>\n"
                        "\n"
                        "typedef enum {\n"
                        "    INT,\n"
                        "    STRING\n"
                        "} Type;\n"
                        "\n"
                        "typedef struct {\n"
                        "    void* data;\n"
                        "    Type type;\n"
                        "} Data;\n")
    );
}

static void write_program_end(
    StrBuffer buffer[static 1], CodeGen code_gen[static 1]
) {
}

void generate(
    CodeGen code_gen[static 1], StrBuffer buffer[static 1],
    AstNode root[static 1]
) {
    write_program_start(buffer, root);

    generate_node(buffer, code_gen, root);

    write_program_end(buffer, code_gen);
}
