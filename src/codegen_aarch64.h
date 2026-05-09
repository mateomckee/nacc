#include "tac.h"
#include <stdio.h>

//8 byte word size
//known limitation: int/chars only need 4 bytes, pointers need 8 bytes, so for MVP, just make everything use 8 bytes, wasting 4 extra bytes on the int/chars
//TODO: add type kind to var entry to know how many bytes to allocate to a given variable
#define WORD_SIZE 8

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
