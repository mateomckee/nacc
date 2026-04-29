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
void tac_node(TACGen* tac, ASTNode* node) {
    if(node == NULL) return;

    printf("tac: %s '%.*s'\n", node_kind_str(node->kind), node->token.length, node->token.start);

    switch(node->kind) {
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
            break;
        }
        case NODE_BLOCK :
            break;
        
        //nodes that generate TAC instructions:
        //func begin/end markers
        case NODE_FUNC :
            break;
        //labels and jumps
        case NODE_IF:
            break;
        case NODE_WHILE :
            break;
        case NODE_FOR :
            break;

        case NODE_DECL : //assignment if initialized
            break;
        case NODE_ASSIGN :
            break;
        case NODE_CALL : //params + call
            break;
        case NODE_RETURN :
            break;
        
        case NODE_BINOP :
            break;
        case NODE_UNOP :
            break;
        case NODE_POSTFIX :
            break;

        //only return operand strings
        case NODE_INT_LIT :
            break;
        case NODE_CHAR_LIT :
            break;
        case NODE_STRING_LIT :
            break;
        case NODE_IDENT :
            break;

        default:
            break;
    }
}

void print_tac(TACGen* tac) {
    printf("TAC:\n");
    int count = tac->count;
    int i;
    for(i = 0; i < count; i++) {
        TACInstr instruction = tac->instructions[i];
        printf("%s = %s op %s\n", instruction.result, instruction.op1, instruction.op2);    
    }
}
