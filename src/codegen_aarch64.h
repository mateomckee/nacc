#include "tac.h"
#include <stdio.h>

#define WORD_SIZE 4   // 32-bit int/char
#define PTR_SIZE  8   // 64-bit pointer

#define FUNC_MAX_VARS 256

/*
 * codegen will use VarTables to keep track of all unique variables in each function, as a pre-pass process
 *
 */



typedef struct {
    char name[TAC_NAME_MAX];
    int offset; //byte offset from stack pointer (sp)

} VarEntry;

typedef struct {
    FILE* out; //output file
    TACGen* tac; //tac to consume

    VarEntry* vars; //var entries (current function)
    char current_func[TAC_NAME_MAX];
    int var_count; //num of vars (current function)
    int frame_size; //total stack frame size (current function)
} CodeGen;

void codegen_init(CodeGen* cg, TACGen* tac);
void codegen_run(CodeGen* cg, const char* output_filename);
void print_codegen(const char* output_filename);
