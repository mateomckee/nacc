#include "codegen_aarch64.h"
#include <stdlib.h>

void codegen_init(CodeGen* cg, TACGen* tac) {
    cg->tac = tac;

    cg->var_count = 0;
    cg->frame_size = 0;
}


//variable table helper functions

//returns byte offset for a given var name
int find_var(CodeGen* cg, char* name) {

}

//adds new var to var table, returns offset
int add_var(CodeGen* cg, char* name) {


}

//scans TAC range (start, end) and builds var table
void build_var_table(CodeGen* cg, int start, int end) {


}

//emit functions

void emit_global(CodeGen* cg, TACInstr) {

}


//main codegen loop
void codegen_run(CodeGen* cg, const char* output_filename) {
    cg->out = fopen(output_filename, "w");
    if(cg->out == NULL) { fprintf(stderr, "could not open file %s\n", output_filename); exit(1); }

    //fist pass: emit all globals and string literals
    int i;
    int instr_count = cg->tac->count;
    for(i = 0; i < instr_count; i++) {
        TACInstr cur_instr = cg->tac->instructions[i];
        if(cur_instr.kind == TAC_GLOBAL) emit_global(cg, cur_instr);
    }

    //emit .text section header
    fprintf(cg->out, ".section .text\n");

    //second pass: emit code
    

    //close immediately after finishing
    fclose(cg->out);
}

