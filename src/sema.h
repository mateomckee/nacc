#include "parser.h"

//known limitation. programs that require more than these maximums are outside of my subset scope
#define MAX_SCOPE_DEPTH 32 //scope stack depth
#define MAX_SCOPE_SYMBOLS 64 //symbols per scope

#define MAX_FUNC 64
#define MAX_PARAMS 8

typedef struct {
    const char* start;
    int length;

    TypeKind return_type;
    TypeKind param_types[MAX_PARAMS];
    int param_count;
} FuncSymbol;

//

typedef struct {
    //name
    const char* start;
    int length;

    TypeKind type;
    int is_global;
} Symbol;

typedef struct {
    struct Scope* parent; //linkedlist pointer to previous scope for closing current scope
    
    int count;
    Symbol symbols[MAX_SCOPE_SYMBOLS];
} Scope;

//contains everything needed for semantic analysis, e.g., scope stack, return type
typedef struct {
    //symbol table
    Scope* current_scope; //most local scope
    TypeKind current_return; //return type of function being walked
    
    //function symbols
    int func_count;
    FuncSymbol functions[MAX_FUNC];
} Sema;

void sema_init(Sema* sema);
void collect_functions(Sema* sema, ASTNode* node);
void sema_node(Sema* sema, ASTNode* node);
