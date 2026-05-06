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

//add instruction to TAC instructions array
void emit(TACGen* tac, TACKind kind, char* result, char* op1, char* op2) {
    //if reached instruction capacity, double the capacity
    if(tac->count >= tac->capacity) {
        tac->capacity *= 2;
        tac->instructions = nacc_realloc(tac->instructions, tac->capacity); //handles out of memory
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
}

//same concept as previous step (semantic analysis), recursively walk the AST (DFS), performing certain TAC generation actions depending on the ASTNode kind
//key concept: statements return nothing, expressions return a result
//walk subexpressions, store results, build the TAC, append to TACGen instructions dynamic array, thats the idea
char* tac_node(TACGen* tac, ASTNode* node) {
    if(node == NULL) return NULL;

    printf("tac: %s '%.*s'\n", node_kind_str(node->kind), node->token.length, node->token.start);

    switch(node->kind) {
        //statement nodes, no return value
        //nodes that just walk the tree:
        case NODE_PROGRAM : {
            //walk global vars
            ASTNode* global_node = node->right;
            while(global_node != NULL) {
                tac_node(tac, global_node);
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
        case NODE_FUNC :
            tac_node(tac, node->left); //walk params

            //begin marker

            tac_node(tac, node->right); //walk body

            //end marker
            return NULL;
        //labels and jumps
        case NODE_IF:
            tac_node(tac, node->left);
            tac_node(tac, node->right);
            tac_node(tac, node->extra);
            
            return NULL;
        case NODE_WHILE :
            return NULL;
        case NODE_FOR :
            return NULL;
        case NODE_RETURN :
            return NULL;
        case NODE_DECL : //assignment if initialized
            return NULL;

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
                case TOK_NOT: kind = TAC_NEQ; break;
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
            if(kind == TAC_ADD || kind == TAC_SUB) {
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
            //assign to: name
            char* name = nacc_malloc(TAC_NAME_MAX);
            snprintf(name, TAC_NAME_MAX, "%.*s", node->left->token.length, node->left->token.start);

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
            emit(tac, TAC_ASSIGN, name, right, NULL);
            return name;
        }
        case NODE_CALL : //params + call
            return NULL;
        //only return operand strings
        case NODE_INT_LIT :
        case NODE_CHAR_LIT :
        case NODE_STRING_LIT :
        case NODE_IDENT : {
            char* result = nacc_malloc(TAC_NAME_MAX);
            snprintf(result, TAC_NAME_MAX, "%.*s", node->token.length, node->token.start);
            return result;
        }
        default:
            return NULL;
    }
}

const char* tac_kind_str(TACKind kind) {
    switch(kind) {
        case TAC_ASSIGN:      return "ASSIGN";
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
        case TAC_PARAM:       return "PARAM";
        case TAC_CALL:        return "CALL";
        case TAC_RETURN:      return "RETURN";
        case TAC_FUNC_BEGIN:  return "FUNC_BEGIN";
        case TAC_FUNC_END:    return "FUNC_END";
        case TAC_GLOBAL:      return "GLOBAL";
        default:              return "UNKNOWN";
    }
}

void print_tac(TACGen* tac) {
    printf("TAC:\n");
    int count = tac->count;
    int i;
    for(i = 0; i < count; i++) {
        TACInstr instruction = tac->instructions[i];
        printf("%s = %s %s %s\n", instruction.result, instruction.op1, tac_kind_str(instruction.kind), instruction.op2);    
    }
}
