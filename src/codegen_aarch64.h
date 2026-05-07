#include "tac.h"
#include <stdio.h>

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
    int var_count; //num of vars (current function)
    int frame_size; //total stack frame size (current function)
} CodeGen;

void codegen_init(CodeGen* cg, TACGen* tac, FILE* out);
void codegen_run(CodeGen* cg);
