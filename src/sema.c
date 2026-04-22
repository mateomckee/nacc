#include <stdio.h>
#include "sema.h"
#include "util.h"

/*
 * Semantic analysis
 * performs an AST tree walk, keeps track of scope via stack, validates and annotates ID nodes
 */

FuncSymbol* make_func_symbol(const char* start, int length, TypeKind return_type) {
    FuncSymbol* func_symbol = nacc_malloc(sizeof(FuncSymbol));
    func_symbol->start = start;
    func_symbol->length = length;

    func_symbol->return_type = return_type;
    func_symbol->param_count = 0;

    return func_symbol;
}

Symbol* make_symbol(const char* start, int length, TypeKind type, int is_global) {
    Symbol* symbol = nacc_malloc(sizeof(Symbol));
    symbol->start = start;
    symbol->length = 0;

    symbol->type = type;
    symbol->is_global = is_global;

    return symbol;
}

Scope* make_scope(Scope* parent) {
    Scope* scope = nacc_malloc(sizeof(Scope));
    scope->parent = parent;
    scope->count = 0;
    return scope;
}

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
        FuncSymbol* func_symbol = make_func_symbol(node->token.start, node->token.length, node->type);
        
        //get params
        ASTNode* node_param = node->left;
        while(node_param != NULL) {   
            printf("param\n");
            if(func_symbol->param_count >= MAX_PARAMS) {
                error(node_param->token.line, "function signature exceeded maximum number of parameters");                
            }
            func_symbol->param_types[func_symbol->param_count++] = node_param->type;

            //move on to sibling param
            node_param = node_param->next;
        }

        //store
        sema->functions[sema->func_count++] = *func_symbol;

        //move on to sibling function
        node = node->next;   
    }

    int i;
    for(i = 0; i < sema->func_count; i++) {
        printf(" '%.*s'", sema->functions[i].length, sema->functions[i].start);
        printf("%d\n", sema->functions[i].param_count);
    }
}
    
//second pass:
//recursive DFS tree walk
void sema_node(Sema* sema, ASTNode* node) {
    if(node == NULL) return;

    //printf(" '%.*s'", node->token.length, node->token.start);

    sema_node(sema, node->left);
    sema_node(sema, node->right);

    sema_node(sema, node->next);
}
