#include "codegen_aarch64.h"
#include <string.h>
#include <stdlib.h>
#include "util.h"

//round up to nearest 16 (required by AArch64)
int align16(int size) {
    return (size + 15) & ~15;
}

//load an operand into a register
//handles both immediate values and variables
void load_operand(CodeGen* cg, char* operand, int reg) {
    if (operand == NULL || *operand == '\0') return;

    //check if it's a number (immediate)
    char* endptr;
    strtol(operand, &endptr, 10);
    if (*endptr == '\0') {
        //immediate value
        fprintf(cg->out, "\tmov w%d, #%s\n", reg, operand);
    } else {
        //variable, load from stack
        int offset = find_var(cg, operand);
        fprintf(cg->out, "\tldr w%d, [sp, #%d]\n", reg, offset);
    }
}

//store a register to a variables stack slot
void store_result(CodeGen* cg, char* result, int reg) {
    if (result == NULL || *result == '\0') return;
    int offset = find_var(cg, result);
    fprintf(cg->out, "\tstr w%d, [sp, #%d]\n", reg, offset);
}

//

void codegen_init(CodeGen* cg, TACGen* tac) {
    cg->vars = nacc_malloc(sizeof(VarEntry) * FUNC_MAX_VARS);
    cg->tac = tac;

    cg->var_count = 0;
    cg->frame_size = 0;
}

//variable table helper functions

int is_var(char* name) {
    if (name == NULL || *name == '\0') return 0;

    //labels start with L
    if (*name == 'L') return 0;

    //string literal labels start with str
    if (strncmp(name, "str", 3) == 0) return 0;

    //all digits, possibly leading minus
    char* endptr;
    strtol(name, &endptr, 10);
    if (*endptr == '\0') return 0;

    return 1;
}

//relaxed lookup
//returns offset if found, returns -1 if not found
int lookup_var(CodeGen* cg, char* name) {
    int count = cg->var_count;
    for(int i = 0; i < count; i++) {
        VarEntry var = cg->vars[i];
        if(strncmp(var.name, name, TAC_NAME_MAX) == 0) { return var.offset; }
    }
    return -1;
}

//strict find
//returns byte offset for a given var, errors out if not found (must be found)
int find_var(CodeGen* cg, char* name) {
    int found = lookup_var(cg, name);
    if(found != -1) { return found; }

    error(-1, "codegen: unknown variable '%s'", name);
    return -1;
}

//adds new var to var table, returns offset
int add_var(CodeGen* cg, char* name) {
    int existing_offset = lookup_var(cg, name);
    if(existing_offset != -1) {
        return existing_offset;
    }

    if(cg->var_count >= FUNC_MAX_VARS) {
        error(-1, "codegen: exceeded maximum variables in function '%s'\n", name);
        return -1;
    }

    //new variable entry
    VarEntry new_var;
    strncpy(new_var.name, name, TAC_NAME_MAX); //set name

    new_var.offset = cg->var_count * WORD_SIZE;

    cg->vars[cg->var_count] = new_var; //assign
    cg->var_count++;

    return new_var.offset;
}

//scans TAC range (start, end) and builds var table
void build_var_table(CodeGen* cg, int start) {
    if(start < 0 || start > cg->tac->count) {
        error(-1, "codegen: invalid start instruction index '%d' for building variable table\n", start);
        return;
    }

    //reset var count for each function
    cg->var_count = 0;
    
    int i = start + 1; //skip FUNC_BEGIN instruction
    TACInstr instr = cg->tac->instructions[i];

    //scan all TAC instrutions in this function
    while(instr.kind != TAC_FUNC_END && i < cg->tac->count) {
        //handle special cases, and use default to handle the rest
        switch(instr.kind) {
            case TAC_CALL :
                if(is_var(instr.result)) add_var(cg, instr.result);
                //op1 is a function name, skip it
                if(is_var(instr.op2)) add_var(cg, instr.op2);
                break;
            case TAC_JUMP : break; //only has result, which is a label, skip entirely
            case TAC_JUMP_FALSE :
                if(is_var(instr.op1)) add_var(cg, instr.op1);
                break;
            case TAC_PARAM_DECL :
                if(is_var(instr.result)) add_var(cg, instr.result);
                break;
            case TAC_ARG :
                if(is_var(instr.result)) add_var(cg, instr.result);
                break;
            case TAC_RETURN :
                if(is_var(instr.op1)) add_var(cg, instr.op1);
                break;
            case TAC_GLOBAL : break; //skip entirely
            default:
                if(is_var(instr.result)) add_var(cg, instr.result);
                if(is_var(instr.op1)) add_var(cg, instr.op1);
                if(is_var(instr.op2)) add_var(cg, instr.op2);
                break;
        }

        //next instruction
        instr = cg->tac->instructions[++i];
    }

    cg->frame_size = align16(cg->var_count * WORD_SIZE + 16);
}

//emit helper functions
void load_var(CodeGen* cg, char* name, char* reg) {
    
}
void store_var(CodeGen* cg, char* name, char* reg) {
    
}

//emit functions

void emit_assign(CodeGen* cg, TACInstr* instr) {

}

void emit_global(CodeGen* cg, TACInstr* instr) {

}
void emit_func_begin(CodeGen* cg, TACInstr* instr, int start) {
    //build variable table for this function
    build_var_table(cg, start);

    //store current func name
    strncpy(cg->current_func, instr->result, TAC_NAME_MAX);

    //emit .global and label
    fprintf(cg->out, ".global %s\n", instr->result);
    fprintf(cg->out, "%s:\n", instr->result);

    //emit prologue
    fprintf(cg->out, "\tstp x29, x30, [sp, #-%d]!\n", cg->frame_size);
    fprintf(cg->out, "\tmov x29, sp\n");

    //spill parameters
    int i = start + 1; //skip func begin
    int reg = 0; //register
    TACInstr param_instr = cg->tac->instructions[i]; 
    while(param_instr.kind == TAC_PARAM_DECL) {
        int offset = find_var(cg, param_instr.result);
        fprintf(cg->out, "\tstr w%d, [sp, #%d]\n", reg, offset);

        reg++;
        param_instr = cg->tac->instructions[++i];
    }

    //
}
void emit_func_end(CodeGen* cg, TACInstr* instr, int end) {
    fprintf(cg->out, "%s_end:\n", instr->result);
    fprintf(cg->out, "\tldp x29, x30, [sp], #%d\n", cg->frame_size);
    fprintf(cg->out, "\tret\n\n");
}

void emit_binop(CodeGen* cg, TACInstr* instr, const char* kind) {

}
void emit_sdiv(CodeGen* cg, TACInstr* instr) {

}

void emit_label(CodeGen* cg, TACInstr* instr) {

}
void emit_jump(CodeGen* cg, TACInstr* instr) {

}
void emit_jump_false(CodeGen* cg, TACInstr* instr) {

}

void emit_call(CodeGen* cg, TACInstr* instr, char arg_buf[][TAC_NAME_MAX], int arg_count) {

}
void emit_return(CodeGen* cg, TACInstr* instr) {

}

void emit_compare(CodeGen* cg, TACInstr* instr) {

}

//main codegen loop
void codegen_run(CodeGen* cg, const char* output_filename) {
    cg->out = fopen(output_filename, "w");
    if(cg->out == NULL) { fprintf(stderr, "could not open file %s\n", output_filename); exit(1); }

    int i;
    int instr_count = cg->tac->count;

    //fist pass: emit all globals and string literals
    for(i = 0; i < instr_count; i++) {
        TACInstr* instr = &cg->tac->instructions[i];
        if(instr->kind == TAC_GLOBAL) emit_global(cg, instr);
    }

    //emit .text section header
    fprintf(cg->out, ".section .text\n");

    char arg_buf[MAX_PARAMS][TAC_NAME_MAX];  //default is max 8 args at a time
    int arg_count = 0;

    //second pass: emit code
    for(i = 0; i < instr_count; i++) {
        TACInstr* instr = &cg->tac->instructions[i];
        
        switch(instr->kind) {
            case TAC_FUNC_BEGIN : emit_func_begin(cg, instr, i); break;
            case TAC_FUNC_END: emit_func_end(cg, instr, i); break;
            case TAC_ASSIGN : emit_assign(cg, instr); break;
            case TAC_ADD : emit_binop(cg, instr, "add"); break;
            case TAC_SUB : emit_binop(cg, instr, "sub"); break;
            case TAC_MUL : emit_binop(cg, instr, "mul"); break;
            case TAC_DIV : emit_sdiv(cg, instr); break;
            case TAC_LABEL : emit_label(cg, instr); break;
            case TAC_JUMP : emit_jump(cg, instr); break;
            case TAC_JUMP_FALSE : emit_jump_false(cg, instr); break;
            case TAC_ARG :
                //load arg into arg_buf
                strncpy(arg_buf[arg_count++], instr->result, TAC_NAME_MAX);
                break;
            case TAC_CALL :
                emit_call(cg, instr, arg_buf, arg_count);
                arg_count = 0; //reset args
                break;
            case TAC_RETURN : emit_return(cg, instr); break;
            case TAC_LT :
            case TAC_GT :
            case TAC_LTE :
            case TAC_GTE :
            case TAC_EQEQ :
            case TAC_NEQ :
            case TAC_AND : //TODO: give AND and OR their own emit function
            case TAC_OR :
                emit_compare(cg, instr);
                break;
            //later
            case TAC_NEG: break;
            case TAC_NOT: break;
            case TAC_DEREF : break;
            case TAC_ADDR : break;

            case TAC_GLOBAL: break; //already passed

            default: break;
        }
    }

    //close immediately after finishing
    fclose(cg->out);
}

