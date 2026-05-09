#include "tac.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

//temp and label helpers
char* make_temp(TACGen* tac) {
    char* temp = nacc_malloc(TAC_NAME_MAX);
    snprintf(temp, TAC_NAME_MAX, "t%d", tac->temp_count++);
    return temp;
}

char* make_label(TACGen* tac) {
    char* label = nacc_malloc(TAC_NAME_MAX);
    snprintf(label, TAC_NAME_MAX, "L%d", tac->label_count++);
    return label;
}

char* make_str(TACGen* tac) {
    char* str = nacc_malloc(TAC_NAME_MAX);
    snprintf(str, TAC_NAME_MAX, "str%d", tac->str_count++);
    return str;
}

//add instruction to TAC instructions array
void emit(TACGen* tac, TACKind kind, char* result, char* op1, char* op2) {
    //if reached instruction capacity, double the capacity
    if(tac->count >= tac->capacity) {
        tac->capacity *= 2;
        tac->instructions = nacc_realloc(tac->instructions, sizeof(TACInstr) * tac->capacity); //handles out of memory
    }

    TACInstr new_instruction;
    new_instruction.kind = kind;
    strncpy(new_instruction.result, result ? result : "", TAC_NAME_MAX-1);
    strncpy(new_instruction.op1, op1 ? op1 : "", TAC_NAME_MAX-1);
    strncpy(new_instruction.op2, op2 ? op2 : "", TAC_NAME_MAX-1);

    tac->instructions[tac->count++] = new_instruction;
}

void tac_init(TACGen* tac) {
    tac->capacity = TAC_CAPACITY;
    tac->instructions = nacc_malloc(sizeof(TACInstr) * tac->capacity);

    tac->count = 0;
    tac->temp_count = 0;
    tac->label_count = 0;
    tac->str_count = 0;
}

//same concept as previous step (semantic analysis), recursively walk the AST (DFS), performing certain TAC generation actions depending on the ASTNode kind
//key concept: statements return nothing, expressions return a result
//walk subexpressions, store results, build the TAC, append to TACGen instructions dynamic array, thats the idea
char* tac_node(TACGen* tac, ASTNode* node) {
    if(node == NULL) return NULL;

    switch(node->kind) {
        //statement nodes, no return value
        //nodes that just walk the tree:
        case NODE_PROGRAM : {
            //walk global vars
            ASTNode* global_node = node->right;
            while(global_node != NULL) {
                char* global_name = nacc_malloc(TAC_NAME_MAX);
                snprintf(global_name, TAC_NAME_MAX, "%.*s", global_node->token.length, global_node->token.start);

                char* init = tac_node(tac, global_node->left);

                emit(tac, TAC_GLOBAL, global_name, init, NULL);

                global_node = global_node->next;
            }

            //walk functions
            ASTNode* func_node = node->left;
            while(func_node != NULL) {
                tac_node(tac, func_node);
                func_node = func_node->next;
            }
            return NULL;
        }
        case NODE_BLOCK : {
            //walk statements
            ASTNode* stmt_node = node->left;
            while(stmt_node != NULL) {
                tac_node(tac, stmt_node);
                stmt_node = stmt_node->next;
            }
            return NULL;
        }
        //nodes that generate TAC instructions:
        //func begin/end markers
        case NODE_FUNC : {
            char* name = nacc_malloc(TAC_NAME_MAX);
            snprintf(name, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);

            //start marker
            emit(tac, TAC_FUNC_BEGIN, name, NULL, NULL);

            //walk param list and emit TAC_PARAM_DECL to make codegen easier
            ASTNode* param_node = node->left;
            while(param_node != NULL) {
                char* param_name = nacc_malloc(TAC_NAME_MAX);
                snprintf(param_name, TAC_NAME_MAX, "%.*s", param_node->token.length, param_node->token.start);

                emit(tac, TAC_PARAM_DECL, param_name, NULL, NULL);
                param_node = param_node->next;
            }

            tac_node(tac, node->right); //walk body

            //end marker 
            emit(tac, TAC_FUNC_END, name, NULL, NULL);

            return NULL;
        }
        //labels and jumps
        case NODE_IF: {
            int hasElse = (node->extra == NULL) ? 0 : 1;

            char* l_else = make_label(tac);
            char* l_end = make_label(tac);
            
            //if no else statement, make the else label the end label
            if(!hasElse) l_else = l_end;

            //get condition
            char* cond = tac_node(tac, node->left);

            //if false, skip to else block (or end if no else block)
            emit(tac, TAC_JUMP_FALSE, l_else, cond, NULL);

            tac_node(tac, node->right);

            emit(tac, TAC_JUMP, l_end, NULL, NULL);

            //only generate this if theres an else block
            if(hasElse) {
                emit(tac, TAC_LABEL, l_else, NULL, NULL);

                tac_node(tac, node->extra);
            }

            emit(tac, TAC_LABEL, l_end, NULL, NULL);

            return NULL;
        }
        case NODE_WHILE : {
            char* l_start = make_label(tac);
            char* l_end = make_label(tac);

            //loop start label
            emit(tac, TAC_LABEL, l_start, NULL, NULL);

            //check condition, jump to end if false, continue to body if true
            char* cond = tac_node(tac, node->left);
            emit(tac, TAC_JUMP_FALSE, l_end, cond, NULL);

            //walk body
            tac_node(tac, node->right);

            //jump to start of loop
            emit(tac, TAC_JUMP, l_start, NULL, NULL);

            //end of loop
            emit(tac, TAC_LABEL, l_end, NULL, NULL);

            return NULL;
        }
        case NODE_FOR : {
            char* l_start = make_label(tac);
            char* l_end = make_label(tac);

            //init
            tac_node(tac, node->left);

            //start loop
            emit(tac, TAC_LABEL, l_start, NULL, NULL);

            char* cond = tac_node(tac, node->right);

            emit(tac, TAC_JUMP_FALSE, l_end, cond, NULL);

            //loop body
            tac_node(tac, node->extra);

            //post
            tac_node(tac, node->extra2);

            //loop
            emit(tac, TAC_JUMP, l_start, NULL, NULL);
            
            //end loop
            emit(tac, TAC_LABEL, l_end, NULL, NULL);

            return NULL;
        }
        case NODE_RETURN : {
            char* val = tac_node(tac, node->left);
            //ok if null, handled in codegen. place in op1, not result
            emit(tac, TAC_RETURN, NULL, val, NULL);
            return NULL;
        }
        case NODE_DECL : {
            //nothing to emit, variable just exists, codegen handles allocation in stack space
            //only handle initialization
            if (node->left != NULL) {
                char* left = tac_node(tac, node->left);

                char* name = nacc_malloc(TAC_NAME_MAX);
                snprintf(name, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);
            
                emit(tac, TAC_ASSIGN, name, left, NULL);    
            }
            return NULL;
        }
        //expression statements, returns operand string
        case NODE_BINOP: {
            char* left = tac_node(tac, node->left);
            char* right = tac_node(tac, node->right);
            char* temp = make_temp(tac);
            
            //pick the right TAC op based on operator token
            TACKind kind;
            switch(node->token.kind) {
                case TOK_PLUS: kind = TAC_ADD; break;
                case TOK_MINUS: kind = TAC_SUB; break;
                case TOK_STAR: kind = TAC_MUL; break;
                case TOK_SLASH: kind = TAC_DIV; break;
                case TOK_LT: kind = TAC_LT;  break;
                case TOK_GT: kind = TAC_GT;  break;
                case TOK_GTE: kind = TAC_GTE; break;
                case TOK_LTE: kind = TAC_LTE; break;
                case TOK_EQEQ: kind = TAC_EQEQ;break;
                case TOK_NEQ: kind = TAC_NEQ; break;
                case TOK_AND: kind = TAC_AND; break;
                case TOK_OR: kind = TAC_OR;  break;
                default: kind = TAC_ADD; break;
            }
    
            emit(tac, kind, temp, left, right);
            return temp;
        }    
        case NODE_UNOP : {
            char* left = tac_node(tac, node->left);
            char* right = NULL;
            char* temp = make_temp(tac);

            //pick TAC unary op
            TACKind kind;
            switch(node->token.kind) {
                case TOK_PLUSPLUS: kind = TAC_ADD; right = "1"; break;
                case TOK_MINUSMINUS: kind = TAC_SUB; right = "1"; break;
                case TOK_STAR: kind = TAC_DEREF; break;
                case TOK_AMPERSAND: kind = TAC_ADDR; break;
                case TOK_MINUS: kind = TAC_NEG ;break;
                case TOK_NOT: kind = TAC_NOT; break;
                default: kind = TAC_ADD; break;
            }

            //emit instruction (for prefix ++/--, increment/decrement goes before assigning)
            emit(tac, kind, temp, left, right);

            //if ++/-- (prefix), assign after increment/decrement
            if(node->token.kind == TOK_PLUSPLUS || node->token.kind == TOK_MINUSMINUS) {
                char* name = nacc_malloc(TAC_NAME_MAX);
                snprintf(name, TAC_NAME_MAX, "%.*s", node->left->token.length, node->left->token.start);
                emit(tac, TAC_ASSIGN, name, temp, NULL);
            }

            return temp;
        }
        case NODE_POSTFIX : {
            char* name = nacc_malloc(TAC_NAME_MAX);
            snprintf(name, TAC_NAME_MAX, "%.*s", node->left->token.length, node->left->token.start);

            //save old value into temp
            char* old = make_temp(tac);
            emit(tac, TAC_ASSIGN, old, name, NULL);

            //get ++/-- kind
            TACKind kind = TAC_NONE;
            switch(node->token.kind) {
                case TOK_PLUSPLUS: kind = TAC_ADD; break;
                case TOK_MINUSMINUS: kind = TAC_SUB; break;
                default: break;
            }
            if(kind == TAC_NONE) {
                error(node->token.line, "unexpected postfix operator kind");
            }

            //compute new value
            char* temp = make_temp(tac);
            emit(tac, kind, temp, name, "1");

            //assign new value
            emit(tac, TAC_ASSIGN, name, temp, NULL);

            //return old value
            return old;
        }
        case NODE_ASSIGN : {
            TACKind tac_kind = TAC_ASSIGN;

            char* name = nacc_malloc(TAC_NAME_MAX);
            //assign operation is dealing with a dereference
            //set name to either the dereferened ptr (name of var ptr is pointing to), or just var name if regular assign
            if(node->left->kind == NODE_UNOP && node->left->token.kind == TOK_STAR) {
                tac_kind = TAC_ASSIGN_DEREF;

                //name should be the pointer variable, not the * token
                snprintf(name, TAC_NAME_MAX, "%.*s", node->left->left->token.length, node->left->left->token.start);
            }
            else { snprintf(name, TAC_NAME_MAX, "%.*s", node->left->token.length, node->left->token.start); }

            //assign from: right
            char* right = tac_node(tac, node->right);

            //handle possible compound assignment operators +=, -=, *=, /=
            TACKind kind = TAC_NONE;
            switch(node->token.kind) {
                case TOK_PLUSEQ: kind = TAC_ADD; break;
                case TOK_MINUSEQ: kind = TAC_SUB; break;
                case TOK_MULTEQ: kind = TAC_MUL; break;
                case TOK_DIVEQ: kind = TAC_DIV; break;
                default: break;
            }
            //if we have a compound assignment operator, first do the arithmetic
            if(kind != TAC_NONE) {
                char* temp = make_temp(tac);
                emit(tac, kind, temp, name, right);
                right = temp; //set right to temp to assign temp to name
            }
            
            //emit final assignment instruction
            emit(tac, tac_kind, name, right, NULL);
            return name;
        }
        case NODE_CALL : {
            char* func_name = nacc_malloc(TAC_NAME_MAX);
            snprintf(func_name, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);

            char temp_args[MAX_PARAMS][TAC_NAME_MAX];

            int argc = 0;
            //emit TAC_ARG for each arg
            ASTNode* arg = node->left;
            while(arg != NULL) {
                char* arg_name = tac_node(tac, arg);
                strncpy(temp_args[argc], arg_name, TAC_NAME_MAX); //copy to temp array
                arg = arg->next;
                argc++;
            }

            //emit TAC after walking all args to generate all their values first
            //this fixes functions being called inbetween args, which messes up codegen return values
            for(int i = 0; i < argc; i++) {
                emit(tac, TAC_ARG, temp_args[i], NULL, NULL);
            }


            //convert argc to string
            char argc_str[TAC_NAME_MAX];
            snprintf(argc_str, TAC_NAME_MAX, "%d", argc);

            //temp carries return value
            char* temp = make_temp(tac);

            //emit TAC_CALL: return value in result, function name in op1, arg count in op2
            emit(tac, TAC_CALL, temp, func_name, argc_str);

            //return temp
            return temp;
        }
        //only return operand strings
        case NODE_INT_LIT :
        case NODE_CHAR_LIT :
        case NODE_IDENT : {
            char* result = nacc_malloc(TAC_NAME_MAX);
            snprintf(result, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);
            return result;
        }
        case NODE_STRING_LIT : {
            char* result = nacc_malloc(TAC_NAME_MAX);
            snprintf(result, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);
            
            //key for string literals, emit as TAC_GLOBALS, and codegen will pick them up and place them in .rodata
            char* str = make_str(tac);
            emit(tac, TAC_GLOBAL, str, result, NULL);
            return str;
        }
        default:
            return NULL;
    }
}

const char* tac_kind_str(TACKind kind) {
    switch(kind) {
        case TAC_ASSIGN:      return "ASSIGN";
        case TAC_ASSIGN_DEREF:return "ASSIGN_DEREF";
        case TAC_ADD:         return "ADD";
        case TAC_SUB:         return "SUB";
        case TAC_MUL:         return "MUL";
        case TAC_DIV:         return "DIV";
        case TAC_LT:          return "LT";
        case TAC_GT:          return "GT";
        case TAC_LTE:         return "LTE";
        case TAC_GTE:         return "GTE";
        case TAC_EQEQ:        return "EQEQ";
        case TAC_NEQ:         return "NEQ";
        case TAC_AND:         return "AND";
        case TAC_OR:          return "OR";
        case TAC_NEG:         return "NEG";
        case TAC_NOT:         return "NOT";
        case TAC_DEREF:       return "DEREF";
        case TAC_ADDR:        return "ADDR";
        case TAC_LABEL:       return "LABEL";
        case TAC_JUMP:        return "JUMP";
        case TAC_JUMP_TRUE:   return "JUMP_TRUE";
        case TAC_JUMP_FALSE:  return "JUMP_FALSE";
        case TAC_ARG:         return "ARG";
        case TAC_CALL:        return "CALL";
        case TAC_RETURN:      return "RETURN";
        case TAC_FUNC_BEGIN:  return "FUNC_BEGIN";
        case TAC_FUNC_END:    return "FUNC_END";
        case TAC_GLOBAL:      return "GLOBAL";
        default:              return "UNKNOWN";
    }
}

void print_tac(TACGen* tac) {
    int i;
    for(i = 0; i < tac->count; i++) {
        TACInstr* in = &tac->instructions[i];
        switch(in->kind) {
            case TAC_FUNC_BEGIN:
                printf("\n[%s]\n", in->result);
                break;
            case TAC_FUNC_END:
                printf("[end %s]\n", in->result);
                break;
            case TAC_LABEL:
                printf("%s:\n", in->result);
                break;
            case TAC_JUMP:
                printf("    goto %s\n", in->result);
                break;
            case TAC_JUMP_FALSE:
                printf("    if !%s goto %s\n", in->op1, in->result);
                break;
            case TAC_JUMP_TRUE:
                printf("    if %s goto %s\n", in->op1, in->result);
                break;
            case TAC_ASSIGN:
                printf("    %s = %s\n", in->result, in->op1);
                break;
            case TAC_ASSIGN_DEREF:
                printf("    %s = %s\n", in->result, in->op1);
                break;
            case TAC_RETURN:
                printf("    return %s\n", in->op1);
                break;
            case TAC_PARAM_DECL:
                printf("    param decl %s\n", in->result);
                break;
            case TAC_ARG:
                printf("    arg %s\n", in->result);
                break;
            case TAC_CALL:
                printf("    %s = call %s (%s args)\n", in->result, in->op1, in->op2);
                break;
            case TAC_GLOBAL:
                printf("    global %s = \"%s\"\n", in->result, in->op1);
                break;
            case TAC_NEG:
                printf("    %s = -%s\n", in->result, in->op1);
                break;
            case TAC_NOT:
                printf("    %s = !%s\n", in->result, in->op1);
                break;
            case TAC_DEREF:
                printf("    %s = *%s\n", in->result, in->op1);
                break;
            case TAC_ADDR:
                printf("    %s = &%s\n", in->result, in->op1);
                break;
            default:
                printf("    %s = %s %s %s\n", in->result, in->op1, tac_kind_str(in->kind), in->op2);
                break;
        }
    }
}
