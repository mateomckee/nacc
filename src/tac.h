#include "sema.h"
#define TAC_CAPACITY 1024
#define TAC_NAME_MAX 64

typedef enum {
    TAC_ASSIGN,

    //arithmetic operators
    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,
    TAC_NEG,

    //logical operators
    TAC_LT,
    TAC_GT,
    TAC_LTE,
    TAC_GTE,
    TAC_EQEQ,
    TAC_NEQ,
    TAC_AND,
    TAC_OR,
    TAC_NOT,
    TAC_DEREF,
    TAC_ADDR,

    //control flow, labels
    TAC_LABEL,
    TAC_JUMP,   //unconditional jump
    TAC_JUMP_TRUE,  //jump if
    TAC_JUMP_FALSE, //jump if not
    TAC_FUNC_BEGIN,
    TAC_FUNC_END,

    TAC_PARAM, //push arg1 as next argument
    TAC_CALL,
    TAC_RETURN,

    TAC_GLOBAL
} TACKind;

//quadruple structure, operator, result, op1, op2
typedef struct {
    //for current MVP, tac operands are stored as just plain strings, but in the future, refactoring to storing them as separate structs would be a better design
    //this is a known limitation, TODO in the future
    TACKind kind; //operator, or operation to perform on op1/op2
    char result[TAC_NAME_MAX]; //destination (temp name, label, variable name)
    char op1[TAC_NAME_MAX];
    char op2[TAC_NAME_MAX];
    
} TACInstr;

typedef struct {
    //instructions are stored as a dynamic array
    TACInstr* instructions;
    //number of instructions
    unsigned int count;
    unsigned int capacity; //alocated size

    //keep track of temporary/label numbers (t0, t1, l0 l1, etc.)
    int temp_count;
    int label_count;
} TACGen;

void tac_init(TACGen* tac);
void tac_node(TACGen* rac, ASTNode* node);
void print_tac(TACGen* tac);
