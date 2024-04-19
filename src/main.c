#include "allocator.h"
#include "analyser.h"
#include "array.h"
#include "ast_render.h"
#include "error_message.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

#include <assert.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <stdio.h>
#include <stdlib.h>

static str_t os_fetch_file(
    allocator_t allocator[static const 1], FILE f[static const 1]
) {
    static size_t const buffer_size = 0x2000;
    str_buffer_t buffer      = str_buffer_new(allocator, buffer_size);
    size_t actual_buffer_length     = 0;
    while (true) {
        size_t amt_read = fread(
            buffer.data + actual_buffer_length, sizeof(char), buffer_size, f
        );
        actual_buffer_length += amt_read;
        buffer.length = actual_buffer_length;
        if (amt_read != buffer_size) {
            if (feof(f)) {
                return str_buffer_str(&buffer);
            } else {
                exit(1);
            }
        }

        str_buffer_ensure_capacity(&buffer, buffer_size);
    }
    vix_unreachable();
}

int main(int const argc, char const* const argv[]) {
    // // Initialize LLVM context and module
    // LLVMContextRef context = LLVMContextCreate();
    // LLVMModuleRef module = LLVMModuleCreateWithName("example");
    //
    // // Define the printf function signature: int32 (i8*, ...)
    // LLVMTypeRef int32Type = LLVMInt32TypeInContext(context);
    // LLVMTypeRef int8PtrType = LLVMPointerType(LLVMInt8TypeInContext(context), 0);
    // LLVMTypeRef printfFuncType = LLVMFunctionType(int32Type, &int8PtrType, 1, 1);
    // 
    // // Declare the printf function
    // LLVMValueRef printfFunc = LLVMAddFunction(module, "printf", printfFuncType);
    // LLVMSetFunctionCallConv(printfFunc, LLVMCCallConv);
    //
    // // Define the function type: void (i8*)
    // LLVMTypeRef voidType = LLVMVoidTypeInContext(context);
    // LLVMTypeRef printFuncParamTypes[] = { int8PtrType };
    // LLVMTypeRef printFuncType = LLVMFunctionType(voidType, printFuncParamTypes, 1, 0);
    //
    // // Create the function named 'printString'
    // LLVMValueRef printStringFunc = LLVMAddFunction(module, "printString", printFuncType);
    // LLVMSetFunctionCallConv(printStringFunc, LLVMCCallConv);
    //
    // // Create a basic block
    // LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlockInContext(context, printStringFunc, "entry");
    // LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    // LLVMPositionBuilderAtEnd(builder, entryBlock);
    //
    // // Get function parameter
    // LLVMValueRef strParam = LLVMGetParam(printStringFunc, 0);
    //
    // // Call printf function to print the string
    // LLVMValueRef formatString = LLVMBuildGlobalStringPtr(builder, "%s\n", "");
    // LLVMValueRef args[] = { formatString, strParam };
    // LLVMValueRef callResult = LLVMBuildCall2(builder, voidType, printfFunc, args, 2, "");
    //
    // // Check if the call result is not null (error checking)
    // LLVMBasicBlockRef exitBlock = LLVMAppendBasicBlockInContext(context, printStringFunc, "exit");
    // LLVMBasicBlockRef continueBlock = LLVMAppendBasicBlockInContext(context, printStringFunc, "continue");
    // LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
    // LLVMValueRef comparison = LLVMBuildICmp(builder, LLVMIntNE, callResult, zero, "cmp");
    // LLVMBuildCondBr(builder, comparison, exitBlock, continueBlock);
    // LLVMPositionBuilderAtEnd(builder, exitBlock);
    // LLVMBuildRetVoid(builder);
    // LLVMPositionBuilderAtEnd(builder, continueBlock);
    //
    // // Return void
    // LLVMBuildRetVoid(builder);
    //
    // // Print out the LLVM IR
    // char *irString = LLVMPrintModuleToString(module);
    // printf("%s\n", irString);
    //
    // // Clean up
    // LLVMDisposeMessage(irString);
    // LLVMDisposeBuilder(builder);
    // LLVMDisposeModule(module);
    // LLVMContextDispose(context);
    if (argc < 2) {
        printf("Please provide a *.vix file to compile");
        exit(1);
    }

    arena_t arena         = {};
    allocator_t allocator = init_arena_allocator(&arena);

    str_t path   = str_new(argv[1]);
    FILE* f             = fopen(path.data, "rb");
    str_t source = os_fetch_file(&allocator, f);
    fclose(f);

    token_ptr_array_t tokens     = array(token_t*, &allocator);
    tokenized_t tokenized = {};
    tokenized.error              = str_new_empty();
    tokenize(&allocator, source, &tokens, &tokenized);

    if (tokenized.error.length != 0) {
        error_message_t const error_message =
            error_message_create_with_line(
                &allocator, path, tokenized.error_line, tokenized.error_column,
                source, tokenized.line_offsets, tokenized.error
            );

        print_error_message(&error_message, ERROR_COLOR_ON);
        exit(1);
    }

    print_tokens(source, *tokenized.tokens);

    import_table_entry_t import_entry = {
        .source_code  = source,
        .line_offsets = tokenized.line_offsets,
        .path         = path,
    };
    import_entry.root = ast_parse(
        &allocator, source, tokenized.tokens, &import_entry, ERROR_COLOR_ON
    );
    assert(import_entry.root != nullptr);

    ast_print(stdout, import_entry.root, 0);

    code_gen_t code_gen = {
        .error_color = ERROR_COLOR_ON,
        .allocator   = &allocator,
        .root_scope =
            {
                         .parent      = nullptr,
                         .source_node = import_entry.root,
                         },
    };
    code_gen.errors                = array(error_message_t*, &allocator);
    code_gen.root_scope.properties = array(ast_node_t*, &allocator);
    code_gen.data_strings          = array(string_data_t, &allocator);

    str_buffer_t buffer = str_buffer_new(&allocator, 0);
    analyse(&code_gen, import_entry.root);
    generate(&code_gen, &buffer, import_entry.root);

    FILE* temp = fopen("vix.c", "w");
    if (temp) {
        fprintf(temp, str_fmt, str_args(buffer));
        fclose(temp);
    }

    system("gcc -O2 -Wall -Wextra -o vix vix.c");

    arena_reset(&arena);
    arena_free(&arena);

    return 0;
}
