#include "codegen_aarch64.h"
#include <string.h>
#include <stdlib.h>
#include "util.h"

//
//type sizing helpers
//

//round up to nearest 16 (required by AArch64)
int align16(int size) {
    return (size + 15) & ~15;
}

static int align_up(int offset, int align) {
    return (offset + align - 1) & ~(align - 1);
}

//byte size of one element of a var/pointer/array type
static int type_size(TypeKind t) {
    switch (t) {
        case TYPE_CHAR:       return 1;
        case TYPE_INT:        return 4;
        case TYPE_INT_PTR:
        case TYPE_CHAR_PTR:
        case TYPE_VOID_PTR:   return 8;
        case TYPE_CHAR_ARRAY: return 1; //element size
        case TYPE_INT_ARRAY:  return 4; //element size
        default:              return 4;
    }
}

static int is_pointer_type(TypeKind t) {
    return t == TYPE_INT_PTR || t == TYPE_CHAR_PTR || t == TYPE_VOID_PTR;
}

//type yielded by *p, given p's type
static TypeKind pointee_of(TypeKind ptr) {
    switch (ptr) {
        case TYPE_INT_PTR:    return TYPE_INT;
        case TYPE_CHAR_PTR:   return TYPE_CHAR;
        case TYPE_INT_ARRAY:  return TYPE_INT;
        case TYPE_CHAR_ARRAY: return TYPE_CHAR;
        default:              return TYPE_INT;
    }
}

//type yielded by &x, given x's type
static TypeKind ptr_to(TypeKind base) {
    switch (base) {
        case TYPE_CHAR:       return TYPE_CHAR_PTR;
        case TYPE_INT:        return TYPE_INT_PTR;
        case TYPE_CHAR_ARRAY: return TYPE_CHAR_PTR;
        case TYPE_INT_ARRAY:  return TYPE_INT_PTR;
        default:              return TYPE_INT_PTR;
    }
}

//
//var/global tables
//

//global table is per-program (built once during the globals pass)
static GlobalEntry globals[MAX_GLOBALS];
static int globals_count = 0;

static GlobalEntry* lookup_global(char* name) {
    for (int i = 0; i < globals_count; i++) {
        if (strncmp(globals[i].name, name, TAC_NAME_MAX) == 0) return &globals[i];
    }
    return NULL;
}

static void register_global(char* name, TypeKind type) {
    if (lookup_global(name) != NULL) return;
    if (globals_count >= MAX_GLOBALS) {
        error(-1, "codegen: exceeded maximum globals: %d", MAX_GLOBALS);
        return;
    }
    strncpy(globals[globals_count].name, name, TAC_NAME_MAX);
    globals[globals_count].type = type;
    globals_count++;
}

static int is_var(char* name) {
    if (name == NULL || *name == '\0') return 0;
    if (*name == 'L') return 0; //labels
    if (strncmp(name, "str", 3) == 0) return 0; //string literal labels
    char* endptr;
    strtol(name, &endptr, 10);
    if (*endptr == '\0') return 0; //numeric immediate
    return 1;
}

static int lookup_var_idx(CodeGen* cg, char* name) {
    for (int i = 0; i < cg->var_count; i++) {
        if (strncmp(cg->vars[i].name, name, TAC_NAME_MAX) == 0) return i;
    }
    return -1;
}

static VarEntry* find_var_entry(CodeGen* cg, char* name) {
    int idx = lookup_var_idx(cg, name);
    if (idx >= 0) return &cg->vars[idx];
    error(-1, "codegen: unknown variable '%s'", name);
    return NULL;
}

//for the rest of codegen, an "operand type" means the declared type of a
//variable/global, or TYPE_NONE for immediates/literals/unknown
static TypeKind operand_type(CodeGen* cg, char* name) {
    if (!is_var(name)) return TYPE_NONE;
    int idx = lookup_var_idx(cg, name);
    if (idx >= 0) return cg->vars[idx].type;
    GlobalEntry* g = lookup_global(name);
    if (g != NULL) return g->type;
    return TYPE_NONE;
}

//returns 1 if name refers to a known global (not a local var/temp)
static int is_global_name(CodeGen* cg, char* name) {
    if (!is_var(name)) return 0;
    if (lookup_var_idx(cg, name) >= 0) return 0;
    return lookup_global(name) != NULL;
}

//adds a new local/temp with explicit type and element count (count > 1 for arrays)
//byte-aligned: each var aligned to its element size (1/4/8)
static int add_var(CodeGen* cg, char* name, TypeKind type, int count) {
    int existing = lookup_var_idx(cg, name);
    if (existing >= 0) return cg->vars[existing].offset;

    if (cg->var_count >= FUNC_MAX_VARS) {
        error(-1, "codegen: exceeded maximum variables in function '%s'", name);
        return -1;
    }

    int elem = type_size(type);
    int size = elem * count;

    cg->frame_used = align_up(cg->frame_used, elem);
    int offset = cg->frame_used;
    cg->frame_used += size;

    VarEntry entry;
    strncpy(entry.name, name, TAC_NAME_MAX);
    entry.offset = offset;
    entry.size = size;
    entry.type = type;
    cg->vars[cg->var_count++] = entry;
    return offset;
}

//infer the result type of an op from its operands, for the var table
static TypeKind infer_result_type(CodeGen* cg, TACInstr* instr) {
    switch (instr->kind) {
        case TAC_ADDR:
            return ptr_to(operand_type(cg, instr->op1));
        case TAC_DEREF:
            return pointee_of(operand_type(cg, instr->op1));
        case TAC_LT: case TAC_GT: case TAC_LTE: case TAC_GTE:
        case TAC_EQEQ: case TAC_NEQ: case TAC_AND: case TAC_OR:
        case TAC_NOT:
            return TYPE_INT;
        case TAC_ASSIGN: {
            TypeKind t = operand_type(cg, instr->op1);
            return (t != TYPE_NONE) ? t : TYPE_INT;
        }
        default: {
            TypeKind t1 = operand_type(cg, instr->op1);
            TypeKind t2 = operand_type(cg, instr->op2);
            if (is_pointer_type(t1)) return t1;
            if (is_pointer_type(t2)) return t2;
            //arrays decay to pointers but for an indexing-style ADD on an array temp, the
            //temp has already been typed as a pointer via TAC_ADDR; here just promote
            if (t1 == TYPE_INT_ARRAY || t2 == TYPE_INT_ARRAY)   return TYPE_INT_PTR;
            if (t1 == TYPE_CHAR_ARRAY || t2 == TYPE_CHAR_ARRAY) return TYPE_CHAR_PTR;
            return TYPE_INT;
        }
    }
}

//register a result var/temp (no-op if already in table or if it's a global)
static void maybe_register_result(CodeGen* cg, TACInstr* instr) {
    char* name = instr->result;
    if (!is_var(name) || is_global_name(cg, name)) return;
    if (lookup_var_idx(cg, name) >= 0) return;
    add_var(cg, name, infer_result_type(cg, instr), 1);
}

//some operands appear before any explicit decl (rare; e.g. accessing a param within a
//paren-expression that produces a temp). Register such vars defensively as int
static void maybe_register_operand(CodeGen* cg, char* name) {
    if (!is_var(name) || is_global_name(cg, name)) return;
    if (lookup_var_idx(cg, name) >= 0) return;
    add_var(cg, name, TYPE_INT, 1);
}

//
//sp-relative load/store helpers (checks width)
//

//AArch64 scaled immediate ranges (positive only)
//ldrb / strb  : 0 to 4095   (no scaling)
//ldr/str w (4) : 0 to 16380  step 4
//ldr/str x (8) : 0 to 32760  step 8

//if the offset doesntt fit, put into x9 and use register addressing
static int fits_imm(int offset, int size) {
    if (offset < 0) return 0;
    switch (size) {
        case 1: return offset <= 4095;
        case 4: return offset <= 16380 && (offset % 4) == 0;
        case 8: return offset <= 32760 && (offset % 8) == 0;
        default: return 0;
    }
}

//emit a sign extending load
static void emit_load(CodeGen* cg, int reg, int offset, int size) {
    const char* op;
    switch (size) {
        case 1: op = "ldrsb"; break;
        case 4: op = "ldrsw"; break;
        case 8: op = "ldr";   break;
        default: error(-1, "codegen: bad load size %d", size); return;
    }
    if (fits_imm(offset, size)) {
        fprintf(cg->out, "\t%s x%d, [sp, #%d]\n", op, reg, offset);
    } else {
        fprintf(cg->out, "\tmov x9, #%d\n", offset);
        fprintf(cg->out, "\t%s x%d, [sp, x9]\n", op, reg);
    }
}

//emit a narrowing store
static void emit_store(CodeGen* cg, int reg, int offset, int size) {
    const char* op;
    const char* w; //register prefix (w for narrow, x for 8-byte)
    switch (size) {
        case 1: op = "strb"; w = "w"; break;
        case 4: op = "str";  w = "w"; break;
        case 8: op = "str";  w = "x"; break;
        default: error(-1, "codegen: bad store size %d", size); return;
    }
    if (fits_imm(offset, size)) {
        fprintf(cg->out, "\t%s %s%d, [sp, #%d]\n", op, w, reg, offset);
    } else {
        fprintf(cg->out, "\tmov x9, #%d\n", offset);
        fprintf(cg->out, "\t%s %s%d, [sp, x9]\n", op, w, reg);
    }
}

static int fits_add_imm(int offset) {
    return offset >= 0 && offset <= 4095;
}

static void emit_add_sp(CodeGen* cg, int reg, int offset) {
    if (fits_add_imm(offset)) {
        fprintf(cg->out, "\tadd x%d, sp, #%d\n", reg, offset);
    } else {
        fprintf(cg->out, "\tmov x9, #%d\n", offset);
        fprintf(cg->out, "\tadd x%d, sp, x9\n", reg);
    }
}

//emit `adrp + add` to put the address of a global into xReg
static void emit_global_addr(CodeGen* cg, int reg, char* name) {
    fprintf(cg->out, "\tadrp x%d, %s\n", reg, name);
    fprintf(cg->out, "\tadd x%d, x%d, :lo12:%s\n", reg, reg, name);
}

//
//operand loading / result storing
//

//load an operand into x{reg} (sign-extended for narrow loads). Handles:
//  - string literal labels (load address)
//  - integer immediates (mov)
//  - globals (adrp+add+load)
//  - local vars / temps (ldr from sp)
//Arrays decay to their address (via TAC_ADDR upstream); here their type is treated as a pointer.
static void load_operand(CodeGen* cg, char* operand, int reg) {
    if (operand == NULL || *operand == '\0') return;

    //string literal: load address
    if (strncmp(operand, "str", 3) == 0) {
        emit_global_addr(cg, reg, operand);
        return;
    }

    //integer immediate
    char* endptr;
    strtol(operand, &endptr, 10);
    if (*endptr == '\0') {
        fprintf(cg->out, "\tmov x%d, #%s\n", reg, operand);
        return;
    }

    //local variable shadows global of the same name; check locals first
    int local_idx = lookup_var_idx(cg, operand);
    if (local_idx >= 0) {
        VarEntry* v = &cg->vars[local_idx];
        emit_load(cg, reg, v->offset, type_size(v->type));
        return;
    }

    //global variable: address then load
    GlobalEntry* g = lookup_global(operand);
    if (g != NULL) {
        //array globals decay to their address (consistent with stack-local array decay)
        if (g->type == TYPE_INT_ARRAY || g->type == TYPE_CHAR_ARRAY) {
            emit_global_addr(cg, reg, operand);
            return;
        }
        emit_global_addr(cg, reg, operand);
        fprintf(cg->out, "\t%s x%d, [x%d]\n",
            type_size(g->type) == 1 ? "ldrsb" :
            type_size(g->type) == 4 ? "ldrsw" : "ldr",
            reg, reg);
        return;
    }

    error(-1, "codegen: unknown variable '%s'", operand);
}

//store x{reg}/w{reg} into the destination variable's slot
static void store_result(CodeGen* cg, char* result, int reg) {
    if (result == NULL || *result == '\0') return;

    //locals shadow globals
    int local_idx = lookup_var_idx(cg, result);
    if (local_idx >= 0) {
        VarEntry* v = &cg->vars[local_idx];
        emit_store(cg, reg, v->offset, v->size);
        return;
    }

    GlobalEntry* g = lookup_global(result);
    if (g != NULL) {
        //compute global address in x9 (avoids clobbering reg)
        emit_global_addr(cg, 9, result);
        int sz = type_size(g->type);
        const char* op = (sz == 1) ? "strb" : "str";
        const char* w  = (sz == 8) ? "x" : "w";
        fprintf(cg->out, "\t%s %s%d, [x9]\n", op, w, reg);
        return;
    }

    error(-1, "codegen: unknown variable '%s'", result);
}

//
//codegen init / globals
//

void codegen_init(CodeGen* cg, TACGen* tac) {
    cg->vars = nacc_malloc(sizeof(VarEntry) * FUNC_MAX_VARS);
    cg->tac = tac;

    cg->var_count = 0;
    cg->frame_used = 0;
    cg->frame_size = 0;
    globals_count = 0;
}

//
//build per-function var table from TAC
//

static void build_var_table(CodeGen* cg, int start) {
    if (start < 0 || start > cg->tac->count) {
        error(-1, "codegen: invalid start instruction index '%d'", start);
        return;
    }

    cg->var_count = 0;
    cg->frame_used = 16; //x29/x30 spill area at [sp, #0]

    int i = start + 1; //skip FUNC_BEGIN
    TACInstr instr = cg->tac->instructions[i];

    while (instr.kind != TAC_FUNC_END && i < cg->tac->count) {
        switch (instr.kind) {
            case TAC_VAR_DECL:
                add_var(cg, instr.result, instr.type, 1);
                break;
            case TAC_PARAM_DECL:
                add_var(cg, instr.result, instr.type, 1);
                break;
            case TAC_ARRAY_DECL: {
                int count = (int)strtol(instr.op1, NULL, 10);
                add_var(cg, instr.result, instr.type, count);
                break;
            }
            case TAC_JUMP: break;
            case TAC_JUMP_FALSE:
                maybe_register_operand(cg, instr.op1);
                break;
            case TAC_CALL:
                maybe_register_result(cg, &instr);
                break;
            case TAC_ARG: break;
            case TAC_RETURN: break;
            case TAC_GLOBAL: break;
            default:
                //register operands first so result-type inference can see them
                maybe_register_operand(cg, instr.op1);
                maybe_register_operand(cg, instr.op2);
                maybe_register_result(cg, &instr);
                break;
        }

        instr = cg->tac->instructions[++i];
    }

    cg->frame_size = align16(cg->frame_used);
}

//
//emit primitives
//

static int arith_width(CodeGen* cg, TACInstr* instr) {
    VarEntry* v = NULL;
    int idx = lookup_var_idx(cg, instr->result);
    if (idx >= 0) v = &cg->vars[idx];
    if (v && v->size == 8) return 8;
    if (is_pointer_type(operand_type(cg, instr->op1))) return 8;
    if (is_pointer_type(operand_type(cg, instr->op2))) return 8;
    return 4;
}

static void emit_assign(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    store_result(cg, instr->result, 0);
}

static void emit_assign_deref(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->result, 0);
    load_operand(cg, instr->op1, 1);

    TypeKind ptr_type = operand_type(cg, instr->result);
    int sz = type_size(pointee_of(ptr_type));
    const char* op = (sz == 1) ? "strb" : "str";
    const char* w  = (sz == 8) ? "x"    : "w";
    fprintf(cg->out, "\t%s %s1, [x0]\n", op, w);
}

static void emit_deref(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);

    TypeKind ptr_type = operand_type(cg, instr->op1);
    int sz = type_size(pointee_of(ptr_type));
    const char* op = (sz == 1) ? "ldrsb" : (sz == 4) ? "ldrsw" : "ldr";
    fprintf(cg->out, "\t%s x0, [x0]\n", op);
    store_result(cg, instr->result, 0);
}

static void emit_addr(CodeGen* cg, TACInstr* instr) {
    //locals shadow globals
    int local_idx = lookup_var_idx(cg, instr->op1);
    if (local_idx >= 0) {
        emit_add_sp(cg, 0, cg->vars[local_idx].offset);
    } else {
        GlobalEntry* g = lookup_global(instr->op1);
        if (g == NULL) { error(-1, "codegen: unknown variable '%s'", instr->op1); return; }
        emit_global_addr(cg, 0, instr->op1);
    }
    store_result(cg, instr->result, 0);
}

//
//globals emission
//

static void emit_string_literal(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, ".section .rodata\n");
    fprintf(cg->out, "%s:\n", instr->result);
    fprintf(cg->out, "\t.string \"%s\"\n", instr->op1);
}

static void emit_global(CodeGen* cg, TACInstr* instr) {
    if (strncmp(instr->result, "str", 3) == 0) {
        emit_string_literal(cg, instr);
        return;
    }

    //track for access from functions
    register_global(instr->result, instr->type);

    fprintf(cg->out, ".section .data\n");
    fprintf(cg->out, "%s:\n", instr->result);

    int has_init = instr->op1[0] != '\0';
    switch (instr->type) {
        case TYPE_CHAR:
            fprintf(cg->out, "\t.byte %s\n", has_init ? instr->op1 : "0");
            break;
        case TYPE_INT:
            fprintf(cg->out, "\t.word %s\n", has_init ? instr->op1 : "0");
            break;
        case TYPE_INT_PTR:
        case TYPE_CHAR_PTR:
        case TYPE_VOID_PTR:
            fprintf(cg->out, "\t.quad %s\n", has_init ? instr->op1 : "0");
            break;
        case TYPE_INT_ARRAY: {
            int count = (int)strtol(instr->op2, NULL, 10);
            fprintf(cg->out, "\t.zero %d\n", count * 4);
            break;
        }
        case TYPE_CHAR_ARRAY: {
            int count = (int)strtol(instr->op2, NULL, 10);
            fprintf(cg->out, "\t.zero %d\n", count);
            break;
        }
        default:
            //fall back to 8 bytes
            fprintf(cg->out, "\t.quad %s\n", has_init ? instr->op1 : "0");
            break;
    }
}

//
//function prologue/epilogue
//

static void emit_func_begin(CodeGen* cg, TACInstr* instr, int start) {
    build_var_table(cg, start);

    strncpy(cg->current_func, instr->result, TAC_NAME_MAX);

    fprintf(cg->out, ".global %s\n", instr->result);
    fprintf(cg->out, "%s:\n", instr->result);

    //stp/ldp pre/post-index for x-pair uses imm7 scaled by 8: range [-512, 504]
    if (cg->frame_size > 504) {
        fprintf(cg->out, "\tsub sp, sp, #%d\n", cg->frame_size);
        fprintf(cg->out, "\tstp x29, x30, [sp]\n");
    } else {
        fprintf(cg->out, "\tstp x29, x30, [sp, #-%d]!\n", cg->frame_size);
    }
    fprintf(cg->out, "\tmov x29, sp\n");

    //spill parameters at their natural width
    int i = start + 1;
    int reg = 0;
    TACInstr p = cg->tac->instructions[i];
    while (p.kind == TAC_PARAM_DECL) {
        VarEntry* v = find_var_entry(cg, p.result);
        emit_store(cg, reg, v->offset, v->size);
        reg++;
        p = cg->tac->instructions[++i];
    }
}

static void emit_func_end(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "%s_end:\n", instr->result);
    if (cg->frame_size > 504) {
        fprintf(cg->out, "\tldp x29, x30, [sp]\n");
        fprintf(cg->out, "\tadd sp, sp, #%d\n", cg->frame_size);
    } else {
        fprintf(cg->out, "\tldp x29, x30, [sp], #%d\n", cg->frame_size);
    }
    fprintf(cg->out, "\tret\n\n");
}

//
//arithmetic/compare/logical/unary
//

static void emit_binop(CodeGen* cg, TACInstr* instr, const char* kind) {
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    const char* w = (arith_width(cg, instr) == 8) ? "x" : "w";
    fprintf(cg->out, "\t%s %s0, %s0, %s1\n", kind, w, w, w);
    store_result(cg, instr->result, 0);
}

static void emit_sdiv(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    const char* w = (arith_width(cg, instr) == 8) ? "x" : "w";
    fprintf(cg->out, "\tsdiv %s0, %s0, %s1\n", w, w, w);
    store_result(cg, instr->result, 0);
}

static void emit_label(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "%s_%s:\n", cg->current_func, instr->result);
}

static void emit_jump(CodeGen* cg, TACInstr* instr) {
    fprintf(cg->out, "\tb %s_%s\n", cg->current_func, instr->result);
}

static void emit_jump_false(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tcbz x0, %s_%s\n", cg->current_func, instr->result);
}

static void emit_call(CodeGen* cg, TACInstr* instr, char arg_buf[][TAC_NAME_MAX], int arg_count) {
    for (int i = 0; i < arg_count; i++) load_operand(cg, arg_buf[i], i);
    fprintf(cg->out, "\tbl %s\n", instr->op1);
    store_result(cg, instr->result, 0);
}

static void emit_return(CodeGen* cg, TACInstr* instr) {
    if (instr->op1[0] != '\0') load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tb %s_end\n", cg->current_func);
}

static void emit_logical(CodeGen* cg, TACInstr* instr, char* kind) {
    //result is always int (4 bytes) — boolean
    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);
    fprintf(cg->out, "\tcmp x0, #0\n");
    fprintf(cg->out, "\tcset w0, ne\n");
    fprintf(cg->out, "\tcmp x1, #0\n");
    fprintf(cg->out, "\tcset w1, ne\n");
    fprintf(cg->out, "\t%s w0, w0, w1\n", kind);
    store_result(cg, instr->result, 0);
}

static void emit_compare(CodeGen* cg, TACInstr* instr) {
    const char* cond = "";
    switch (instr->kind) {
        case TAC_LT:   cond = "lt"; break;
        case TAC_GT:   cond = "gt"; break;
        case TAC_LTE:  cond = "le"; break;
        case TAC_GTE:  cond = "ge"; break;
        case TAC_EQEQ: cond = "eq"; break;
        case TAC_NEQ:  cond = "ne"; break;
        default: error(-1, "codegen: invalid compare kind"); break;
    }

    load_operand(cg, instr->op1, 0);
    load_operand(cg, instr->op2, 1);

    //compare in the wider of operand widths (so pointer compares work)
    int w8 = is_pointer_type(operand_type(cg, instr->op1)) ||
             is_pointer_type(operand_type(cg, instr->op2));
    const char* w = w8 ? "x" : "w";
    fprintf(cg->out, "\tcmp %s0, %s1\n", w, w);
    fprintf(cg->out, "\tcset w0, %s\n", cond);
    store_result(cg, instr->result, 0);
}

static void emit_neg(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    const char* w = (arith_width(cg, instr) == 8) ? "x" : "w";
    fprintf(cg->out, "\tneg %s0, %s0\n", w, w);
    store_result(cg, instr->result, 0);
}

static void emit_not(CodeGen* cg, TACInstr* instr) {
    load_operand(cg, instr->op1, 0);
    fprintf(cg->out, "\tcmp x0, #0\n");
    fprintf(cg->out, "\tcset w0, eq\n");
    store_result(cg, instr->result, 0);
}

//
//main loop
//

void codegen_run(CodeGen* cg, const char* output_filename) {
    cg->out = fopen(output_filename, "w");
    if (cg->out == NULL) { fprintf(stderr, "could not open file %s\n", output_filename); exit(1); }

    int instr_count = cg->tac->count;

    //first pass: emit all globals and string literals (also registers globals for later access)
    for (int i = 0; i < instr_count; i++) {
        TACInstr* instr = &cg->tac->instructions[i];
        if (instr->kind == TAC_GLOBAL) emit_global(cg, instr);
    }

    fprintf(cg->out, ".section .text\n");

    char arg_buf[MAX_PARAMS][TAC_NAME_MAX];
    int arg_count = 0;

    //second pass: emit code
    for (int i = 0; i < instr_count; i++) {
        TACInstr* instr = &cg->tac->instructions[i];
        switch (instr->kind) {
            case TAC_FUNC_BEGIN: emit_func_begin(cg, instr, i); break;
            case TAC_FUNC_END: emit_func_end(cg, instr); break;
            case TAC_ASSIGN: emit_assign(cg, instr); break;
            case TAC_ASSIGN_DEREF: emit_assign_deref(cg, instr); break;
            case TAC_ADD: emit_binop(cg, instr, "add"); break;
            case TAC_SUB: emit_binop(cg, instr, "sub"); break;
            case TAC_MUL: emit_binop(cg, instr, "mul"); break;
            case TAC_DIV: emit_sdiv(cg, instr); break;
            case TAC_LABEL: emit_label(cg, instr); break;
            case TAC_JUMP: emit_jump(cg, instr); break;
            case TAC_JUMP_FALSE: emit_jump_false(cg, instr); break;
            case TAC_ARG:
                strncpy(arg_buf[arg_count++], instr->result, TAC_NAME_MAX);
                break;
            case TAC_CALL:
                emit_call(cg, instr, arg_buf, arg_count);
                arg_count = 0;
                break;
            case TAC_RETURN: emit_return(cg, instr); break;
            case TAC_LT:
            case TAC_GT:
            case TAC_LTE:
            case TAC_GTE:
            case TAC_EQEQ:
            case TAC_NEQ:
                emit_compare(cg, instr); break;
            case TAC_AND: emit_logical(cg, instr, "and"); break;
            case TAC_OR: emit_logical(cg, instr, "orr"); break;
            case TAC_NEG: emit_neg(cg, instr); break;
            case TAC_NOT: emit_not(cg, instr); break;
            case TAC_DEREF: emit_deref(cg, instr); break;
            case TAC_ADDR: emit_addr(cg, instr); break;

            case TAC_GLOBAL: break; //emitted in first pass
            case TAC_VAR_DECL: break;
            case TAC_ARRAY_DECL: break;

            default: break;
        }
    }

    fclose(cg->out);
}
