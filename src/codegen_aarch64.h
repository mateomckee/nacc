#include "tac.h"
#include <stdio.h>

#define MAX_GLOBALS 128
#define FUNC_MAX_VARS 256

/*
 * codegen will use VarTables to keep track of all unique variables in each function, as a pre-pass process
 *
 */

typedef struct {
    char name[TAC_NAME_MAX];
    TypeKind type;
} GlobalEntry;

typedef struct {
    char name[TAC_NAME_MAX];
    int offset;     //byte offset from stack pointer (sp)
    int size;       //total bytes occupied (size of one element * count)
    TypeKind type;  //scalar type for scalars/pointers, element type for arrays (TYPE_INT/CHAR/*_PTR)
} VarEntry;

typedef struct {
    FILE* out; //output file
    TACGen* tac; //tac to consume

    VarEntry* vars; //var entries (current function)
    char current_func[TAC_NAME_MAX];
    int var_count;  //num of vars (current function)
    int frame_used; //bytes consumed by var allocations (pre-16-alignment)
    int frame_size; //total stack frame size, aligned to 16
} CodeGen;

void codegen_init(CodeGen* cg, TACGen* tac);
void codegen_run(CodeGen* cg, const char* output_filename);
void print_codegen(const char* output_filename);
