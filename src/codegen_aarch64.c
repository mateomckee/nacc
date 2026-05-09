#include "codegen_aarch64.h"
#include <string.h>
#include <stdlib.h>
#include "util.h"

int find_var(CodeGen* cg, char* name);

//round up to nearest 16 (required by AArch64)
int align16(int size) {
    return (size + 15) & ~15;
}

//load an operand into a register
//handles both immediate values and variables
void load_operand(CodeGen* cg, char* operand, int reg) {
    if (operand == NULL || *operand == '\0') return;

    //check if its a string
    if (strncmp(operand, "str", 3) == 0) {
        //string literal, load address using adrp/add
        fprintf(cg->out, "\tadrp x0, %s\n", operand);
        fprintf(cg->out, "\tadd x0, x0, :lo12:%s\n", operand);
        return;
    }

    //check if it's a number (immediate)
    char* endptr;
    strtol(operand, &endptr, 10);
    if (*endptr == '\0') {
        //immediate value
        fprintf(cg->out, "\tmov x%d, #%s\n", reg, operand);
    } else {
        //variable, load from stack
        int offset = find_var(cg, operand);
        fprintf(cg->out, "\tldr x%d, [sp, #%d]\n", reg, offset);
    }
}

//store a register to a variables stack slot
void store_result(CodeGen* cg, char* result, int reg) {
    if (result == NULL || *result == '\0') return;
    int offset = find_var(cg, result);
    fprintf(cg->out, "\tstr x%d, [sp, #%d]\n", reg, offset);
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

    new_var.offset = cg->var_count * WORD_SIZE + 16; //plus 16 to not overwrite x29 and x30 (8 bytes each)

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

//emit functions
//1. load operands
//2. emit operation
//3. store result

void emit_assign(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    store_result(cg, instr->result, 0);
}

//instr->result = pointer variable name
//instr->op1 = value to store through pointer
void emit_assign_deref(CodeGen* cg, TACInstr* instr) {
    int ptr_offset = find_var(cg, instr->result);
    fprintf(cg->out, "\tldr x0, [sp, #%d]\n", ptr_offset);  
    load_operand(cg, instr->op1, 1);
    fprintf(cg->out, "\tstr x1, [x0]\n");
}

void emit_global(CodeGen* cg, TACInstr* instr) {
    //section, rodata, and data can be emitted multiple times
    //string
    if(strncmp(instr->result, "str", 3) == 0) {
        fprintf(cg->out, ".section .rodata\n");
        fprintf(cg->out, "%s:\n", instr->result);           //str01:
        fprintf(cg->out, "\t.string \"%s\"\n", instr->op1); //  .string "hi"
    }
    //global variable
    else {
        fprintf(cg->out, ".section .data\n");
        fprintf(cg->out, "%s:\n", instr->result);
        fprintf(cg->out, "\t.word %s\n", instr->op1);
    }
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
        fprintf(cg->out, "\tstr x%d, [sp, #%d]\n", reg, offset);

        reg++;
        param_instr = cg->tac->instructions[++i];
    }
}
void emit_func_end(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "%s_end:\n", instr->result);
    fprintf(cg->out, "\tldp x29, x30, [sp], #%d\n", cg->frame_size);
    fprintf(cg->out, "\tret\n\n");
}

void emit_binop(CodeGen* cg, TACInstr* instr, const char* kind) {
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    fprintf(cg->out, "\t%s x0, x0, x1\n", kind);
    store_result(cg, instr->result, 0);
}
void emit_sdiv(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    fprintf(cg->out, "\tsdiv x0, x0, x1\n");
    store_result(cg, instr->result, 0);
}

void emit_label(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "%s_%s:\n", cg->current_func, instr->result);
}
void emit_jump(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "\tb %s_%s\n", cg->current_func, instr->result);
}
void emit_jump_false(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tcbz x0, %s_%s\n", cg->current_func, instr->result);
}

void emit_call(CodeGen* cg, TACInstr* instr, char arg_buf[][TAC_NAME_MAX], int arg_count) {
    int i;
    for(i = 0; i < arg_count; i++) { load_operand(cg, arg_buf[i], i); }
    
    fprintf(cg->out, "\tbl %s\n", instr->op1);
    store_result(cg, instr->result, 0);
}
void emit_return(CodeGen* cg, TACInstr* instr) {
    //store return value in w0 if any
    if(instr->op1[0] != '\0') { load_operand(cg, instr->op1, 0); }
    fprintf(cg->out, "\tb %s_end\n", cg->current_func);
}

void emit_logical(CodeGen* cg, TACInstr* instr, char* kind) {
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    fprintf(cg->out, "\tcmp x0, #0\n");
    fprintf(cg->out, "\tcset x0, ne\n"); //w0 = (a != 0)
    fprintf(cg->out, "\tcmp x1, #0\n");
    fprintf(cg->out, "\tcset x1, ne\n"); //w1 = (b != 0)
    fprintf(cg->out, "\t%s x0, x0, x1\n", kind);
    store_result(cg, instr->result, 0);
}

void emit_compare(CodeGen* cg, TACInstr* instr) {
    char* cond = "";
    switch(instr->kind) {
        case TAC_LT : cond = "lt"; break;
        case TAC_GT : cond = "gt"; break;
        case TAC_LTE : cond = "le"; break;
        case TAC_GTE : cond = "ge"; break;
        case TAC_EQEQ : cond = "eq"; break;
        case TAC_NEQ : cond = "ne"; break;
        default:
            error(-1, "codegen: invalid compare kind");
            break;
    }

    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    fprintf(cg->out, "\tcmp x0, x1\n");
    fprintf(cg->out, "\tcset x0, %s\n", cond);
    store_result(cg, instr->result, 0);
}

void emit_neg(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tneg x0, x0\n");
    store_result(cg, instr->result, 0);
}

void emit_not(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tcmp x0, #0\n");
    fprintf(cg->out, "\tcset x0, eq\n");
    store_result(cg, instr->result, 0);
}

//pointers require 8 bytes to store, use x instead of w
void emit_addr(CodeGen* cg, TACInstr* instr) {
    int offset = find_var(cg, instr->op1);
    fprintf(cg->out, "\tadd x0, sp, #%d\n", offset);
    int result_offset = find_var(cg, instr->result);
    fprintf(cg->out, "\tstr x0, [sp, #%d]\n", result_offset);
}

void emit_deref(CodeGen* cg, TACInstr* instr) {
    int offset = find_var(cg, instr->op1);
    fprintf(cg->out, "\tldr x0, [sp, #%d]\n", offset);
    fprintf(cg->out, "\tldr x0, [x0]\n");
    int result_offset = find_var(cg, instr->result);
    fprintf(cg->out, "\tstr x0, [sp, #%d]\n", result_offset);
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
            case TAC_FUNC_END: emit_func_end(cg, instr); break;
            case TAC_ASSIGN : emit_assign(cg, instr); break;
            case TAC_ASSIGN_DEREF : emit_assign_deref(cg, instr); break;
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
                emit_compare(cg, instr);
                break;
            case TAC_AND :
                emit_logical(cg, instr, "and");
                break;
            case TAC_OR :
                emit_logical(cg, instr, "orr");
                break;
            case TAC_NEG : emit_neg(cg, instr); break;
            case TAC_NOT : emit_not(cg, instr); break;
            case TAC_DEREF : emit_deref(cg, instr); break;
            case TAC_ADDR : emit_addr(cg, instr); break;

            case TAC_GLOBAL: break; //already passed

            default: break;
        }
    }

    //close immediately after finishing
    fclose(cg->out);
}

