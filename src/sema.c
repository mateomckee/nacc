#include <stdio.h>
#include "sema.h"
#include "util.h"

/*
 * Semantic analysis
 * performs an AST tree walk, keeps track of scope via stack, validates and annotates ID nodes
 */

//symbol helpers

FuncSymbol make_func_symbol(const char* start, int length, TypeKind return_type) {
    FuncSymbol* func_symbol = nacc_malloc(sizeof(FuncSymbol));
    func_symbol->start = start;
    func_symbol->length = length;

    func_symbol->return_type = return_type;
    func_symbol->param_count = 0;

    return *func_symbol;
}

Symbol make_symbol(const char* start, int length, TypeKind type, int is_global) {
    Symbol* symbol = nacc_malloc(sizeof(Symbol));
    symbol->start = start;
    symbol->length = length;

    symbol->type = type;
    symbol->is_global = is_global;

    return *symbol;
}

//scope helpers

Scope* make_scope(Scope* parent) {
    Scope* scope = nacc_malloc(sizeof(Scope));
    scope->parent = parent;
    scope->count = 0;
    return scope;
}

void pop_scope(Sema* sema) {
    sema->current_scope = sema->current_scope->parent;
}

void push_scope(Sema* sema) {
    Scope* scope = make_scope(sema->current_scope);
    sema->current_scope = scope;
}

//AST walk actions

//creates and places a new symbol into the scope stack at the current scope
void declare_symbol(Sema* sema, Symbol symbol) {
   
    printf(" decl '%.*s'", symbol.length, symbol.start);
}

//searches scope stack for declared symbol
void lookup_symbol(Sema* sema, const char* start, int length) {
    
}

//searches sema->functions array for a declare function symbol
void lookup_function(Sema* sema, const char* start, int length) {
    
}

//

void sema_init(Sema* sema) {
    //create global scope
    sema->current_scope = make_scope(NULL); //global scope has no parent
    sema->current_return = TYPE_NONE;
    
    sema->func_count = 0;
}

//first pass:
//collect function symbol information
void collect_functions(Sema* sema, ASTNode* root) {
    ASTNode* node = root->left;

    while(node != NULL) {
        //load
        FuncSymbol func_symbol = make_func_symbol(node->token.start, node->token.length, node->type);
        
        //get params
        ASTNode* node_param = node->left;
        while(node_param != NULL) {   
            if(func_symbol.param_count >= MAX_PARAMS) {
                error(node_param->token.line, "function signature exceeded maximum number of parameters");                
            }
            func_symbol.param_types[func_symbol.param_count++] = node_param->type;

            //move on to sibling param
            node_param = node_param->next;
        }

        //store
        if(sema->func_count >= MAX_FUNC) {
            error(node->token.line, "program exceeded maximum number of functions");
        }
        sema->functions[sema->func_count++] = func_symbol;

        //move on to sibling function
        node = node->next;   
    }
}
    
//second pass:
//recursive DFS tree walk
void sema_node(Sema* sema, ASTNode* node) {
    if(node == NULL) return;

    printf(" '%.*s'", node->token.length, node->token.start);

    switch(node->kind) {
        //simply start by walking first function and first global variable
        case NODE_PROGRAM :
            //first walk global variables
            sema_node(sema, node->right);
            //then walk functions
            sema_node(sema, node->left);    
            break;
        //set current_return, push scope for params, declare params, walk body, pop scope
        case NODE_FUNC:
            //walk sibling function, same scope
            sema_node(sema, node->next);

            //sets current_return to the function ASTNode's type parameter, used specifically for this purpose
            sema->current_return = node->type;

            push_scope(sema);

            //declare function parameters for the function scope
            ASTNode* node_param = node->left;
            while(node_param != NULL) {
                declare_symbol(sema, make_symbol(node_param->token.start, node_param->token.length, node_param->type, 0));
                node_param = node_param->next;
            }

            //walk function body
            sema_node(sema, node->right);

            //pop scope
            pop_scope(sema);

            break;
        //push scope for local vars, walk statements
        case NODE_BLOCK:
            
            break;
        //IF/WHILE/FOR call sema_node() on NODE_BLOCK, which handles push/pop scopes
        case NODE_IF :
            
            break;
        case NODE_WHILE :
            
            break;
        case NODE_FOR :
            
            break;
        case NODE_RETURN :
            
            break;
        case NODE_DECL :
            
            break;
        case NODE_ASSIGN :
            
            break;
        case NODE_BINOP :
            
            break;
        case NODE_UNOP :
            
            break;
        case NODE_CALL :
            
            break;
        case NODE_IDENT :
            
            break;
        case NODE_POSTFIX :
            
            break;
        case NODE_INT_LIT :
            
            break;
        case NODE_CHAR_LIT :
            
            break;
        case NODE_STRING_LIT :
            
            break;
        default:
            error(node->token.line, "unknown AST node kind");
            break;
    }
}
