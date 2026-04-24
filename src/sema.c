#include <stdio.h>
#include "sema.h"
#include <string.h>
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
    sema->depth--;
}

void push_scope(Sema* sema) {
    if(sema->depth >= MAX_SCOPE_DEPTH) {
        error(sema->current_line, "program exceeded maximum scope depth: %d", MAX_SCOPE_DEPTH);
    }
    Scope* scope = make_scope(sema->current_scope);
    sema->current_scope = scope;
    sema->depth++;
}

//AST walk actions

//searches scope stack for declared symbol
Symbol* lookup_symbol(Sema* sema, const char* start, int length) {
    Scope* scope = sema->current_scope;

    //iterate through all scopes, starting at most local scope down to global scope
    while(scope != NULL) {
        //in this scope, iterate through all symbols and look for a match
        int count = scope->count;
        int i;
        for(i = 0; i < count; i++) {
            Symbol symbol_at = scope->symbols[i];
            
            //compare strings and length for a perfect match
            if(symbol_at.length == length && strncmp(symbol_at.start, start, length) == 0) {
                return &scope->symbols[i];
            }
        }
        
        //next higher scope
        scope = scope->parent;
    }
    return NULL;
}

//creates and places a new symbol into the scope stack at the current scope
void declare_symbol(Sema* sema, Symbol symbol) {
    Symbol* found_symbol = lookup_symbol(sema, symbol.start, symbol.length);
    if(found_symbol != NULL) {
        error(sema->current_line, "redeclaration of '%.*s'", symbol.length, symbol.start);

    }
    if(sema->current_scope->count >= MAX_SCOPE_SYMBOLS) {
        error(sema->current_line, "program exceeded maximum symbols in a single scope: %d", MAX_SCOPE_SYMBOLS);
    }
    sema->current_scope->symbols[sema->current_scope->count++] = symbol;
}

//searches sema->functions array for a declare function symbol
FuncSymbol* lookup_function(Sema* sema, const char* start, int length) {
    FuncSymbol* func_symbol = NULL;

    return func_symbol;
}

//

void sema_init(Sema* sema) {
    //create global scope
    sema->current_scope = make_scope(NULL); //global scope has no parent
    sema->current_return = TYPE_NONE;
    sema->depth = 0;
    
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
                error(node_param->token.line, "function signature exceeded maximum number of parameters: %d", MAX_PARAMS);                
            }
            func_symbol.param_types[func_symbol.param_count++] = node_param->type;

            //move on to sibling param
            node_param = node_param->next;
        }

        //store
        if(sema->func_count >= MAX_FUNC) {
            error(node->token.line, "program exceeded maximum number of functions: %d", MAX_FUNC);
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

    printf("sema: %s '%.*s'\n", node_kind_str(node->kind), node->token.length, node->token.start);

    sema->current_line = node->token.line;

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

            //walk sibling function, same scope
            sema_node(sema, node->next);

            break;
        //push scope for local vars, walk statements
        case NODE_BLOCK:
            push_scope(sema);
            
            //walk all statements
            ASTNode* node_statement = node->left;
            while(node_statement != NULL) {
                 sema_node(sema, node_statement);   
                 node_statement = node_statement->next;
            }

            pop_scope(sema);
            break;
        //IF/WHILE/FOR call sema_node() on NODE_BLOCK, which handles push/pop scopes
        case NODE_IF :
            sema_node(sema, node->left);
            sema_node(sema, node->right);
            sema_node(sema, node->extra);
            break;
        case NODE_WHILE :
            sema_node(sema, node->left);            
            sema_node(sema, node->right);            
            break;
        case NODE_FOR :
            sema_node(sema, node->left);            
            sema_node(sema, node->right);            
            sema_node(sema, node->extra);            
            sema_node(sema, node->extra2);            
            break;
        case NODE_RETURN :
            sema_node(sema, node->left);

            //check for matching return types (function and return statement)
            if(sema->current_return != node->left->type) {
                error(node->token.line, "mismatched return types");
            }

            break;
        case NODE_DECL : {
            int is_global = sema->depth == 0 ? 1 : 0;
            //declare symbol (handles duplicates)
            declare_symbol(sema, make_symbol(node->token.start, node->token.length, node->type, is_global));
            //walk initialization if any
            sema_node(sema, node->left);
            break;
        }
        case NODE_ASSIGN :
            sema_node(sema, node->left);            
            sema_node(sema, node->right);            

            //check type compatability between left and right

            //annotate node type with left nodes type

            break;
        case NODE_BINOP :
            sema_node(sema, node->left);
            sema_node(sema, node->right);

            //determine result type from operands

            //annotate node type

            break;
        case NODE_UNOP : 
            sema_node(sema, node->left);

            //determine type based on operator and operand type

            //annotate node type

            break;
        case NODE_CALL : {
            FuncSymbol* func_symbol = lookup_function(sema, node->token.start, node->token.length);
            if(func_symbol == NULL) {
                error(node->token.line, "undeclared function '%.*s'", node->token.length, node->token.start);
            }

            //check argument count matches parameter count

            //walk each argument and check type against parameter type

            //annotate node type with function return type
           
        }
            break;
        case NODE_IDENT : {
            Symbol* symbol = lookup_symbol(sema, node->token.start, node->token.length);
            if(symbol == NULL) {
                error(node->token.line, "undeclared variable '%.*s'", node->token.length, node->token.start);
            }
            //annotate node with node type
            node->type = symbol->type;

            break;
        }
        case NODE_POSTFIX :
            sema_node(sema, node->left);

            //annotate same type as operand 

            break;
        case NODE_INT_LIT :
            node->type = TYPE_INT;

            break;
        case NODE_CHAR_LIT :
            node->type = TYPE_CHAR;

            break;
        case NODE_STRING_LIT :
            node->type = TYPE_CHAR_PTR;

            break;
        default:
            error(node->token.line, "unknown AST node kind");
            break;
    }
}
