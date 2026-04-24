#include <stdio.h>
#include "sema.h"
#include <string.h>
#include "util.h"

/*
 * Semantic analysis
 * performs an AST tree walk, keeps track of scope via stack, validates and annotates ID nodes
 */

int types_compatible(TypeKind a, TypeKind b) {
    if (a == b) return 1;
    //char and int are compatible
    if ((a == TYPE_INT && b == TYPE_CHAR) || (a == TYPE_CHAR && b == TYPE_INT)) return 1;
    return 0;
}

//symbol helpers

FuncSymbol make_func_symbol(const char* start, int length, TypeKind return_type) {
    FuncSymbol func_symbol;
    func_symbol.start = start;
    func_symbol.length = length;

    func_symbol.return_type = return_type;
    func_symbol.param_count = 0;

    return func_symbol;
}

Symbol make_symbol(const char* start, int length, TypeKind type, int is_global) {
    Symbol symbol;
    symbol.start = start;
    symbol.length = length;

    symbol.type = type;
    symbol.is_global = is_global;

    return symbol;
}

//scope helpers

Scope* make_scope(Scope* parent) {
    Scope* scope = nacc_malloc(sizeof(Scope));
    scope->parent = parent;
    scope->count = 0;
    return scope;
}

void pop_scope(Sema* sema) {
    if(sema->current_scope->parent == NULL) {
        error(sema->current_line, "attempting to pop global scope");
    }
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

Symbol* lookup_symbol_in_scope(Scope* scope, const char* start, int length) {
    int count = scope->count;
    int i;
    for(i = 0; i < count; i++) {
        Symbol symbol_at = scope->symbols[i];
        if(symbol_at.length == length && strncmp(symbol_at.start, start, length) == 0) {
            return &scope->symbols[i];
        }
    }

    return NULL;
}

//searches scope stack for declared symbol
Symbol* lookup_symbol(Sema* sema, const char* start, int length) {
    Scope* scope = sema->current_scope;

    //iterate through all scopes, starting at most local scope down to global scope
    while(scope != NULL) {
        Symbol* found_symbol = lookup_symbol_in_scope(scope, start, length);
        if(found_symbol != NULL) {
            return found_symbol;
        }

        //next higher scope
        scope = scope->parent;
    }
    return NULL;
}

//creates and places a new symbol into the scope stack at the current scope
void declare_symbol(Sema* sema, Symbol symbol) {
    Symbol* found_symbol = lookup_symbol_in_scope(sema->current_scope, symbol.start, symbol.length);
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
    int count = sema->func_count;
    int i;
    for(i = 0; i < count; i++) {
        FuncSymbol symbol_at = sema->functions[i];

        if(symbol_at.length == length && strncmp(symbol_at.start, start, length) == 0) {
            return &sema->functions[i];
        }
    }

    return NULL;
}

//

void sema_init(Sema* sema) {
    //create global scope
    sema->current_scope = make_scope(NULL); //global scope has no parent
    sema->current_return = TYPE_NONE;
    sema->current_func_name_len = 0;
    sema->depth = 0;
    
    sema->func_count = 0;

    //manually add printf/scanf function symbols for later use. this is just for my subset of C

    FuncSymbol printf_symbol = make_func_symbol("printf", 6, TYPE_INT);
    printf_symbol.param_count = -1; //variadic function
    sema->functions[sema->func_count++] = printf_symbol;

    FuncSymbol scanf_symbol = make_func_symbol("scanf", 5, TYPE_INT);
    scanf_symbol.param_count = -1; //variadic function
    sema->functions[sema->func_count++] = scanf_symbol;

}

//first pass:
//collect function symbol information
void collect_functions(Sema* sema, ASTNode* root) {
    ASTNode* node = root->left;

    while(node != NULL) {
        if(sema->func_count >= MAX_FUNC) {
            error(node->token.line, "program exceeded maximum number of functions: %d", MAX_FUNC);
        }

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
        case NODE_PROGRAM : {
            //first walk global variables
            ASTNode* global = node->right;
            while(global != NULL) {
                sema_node(sema, global);
                global = global->next;
            }

            //then walk functions
            ASTNode* func = node->left;
            while(func != NULL) {
                sema_node(sema, func);
                func = func->next;
            }
            break;
        }
        //set current_return, push scope for params, declare params, walk body, pop scope
        case NODE_FUNC:
            //sets current_return to the function ASTNode's type parameter, used specifically for this purpose
            //also sets func name for debugging purposes
            sema->current_func_name = node->token.start;
            sema->current_func_name_len = node->token.length;
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
            if(node->left != NULL && sema->current_return != node->left->type) {
                error(node->token.line, "return type does not match function '%.*s', expected %s but got %s", sema->current_func_name_len, sema->current_func_name, type_kind_str(sema->current_return), type_kind_str(node->left->type));
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
            if(!types_compatible(node->left->type, node->right->type)) {
                error(node->token.line, "incompatible types in assignment");
            }
                
            //annotate node type with left nodes type
            node->type = node->left->type;

            break;
        case NODE_BINOP :
            sema_node(sema, node->left);
            sema_node(sema, node->right);

            //determine result type from operands
            //currently, all paths return TYPE_INT, but i built this switch for future extensibility (pointer arithmetic, etc)
            switch(node->token.kind) {
                case TOK_LT:
                    node->type = TYPE_INT;
                    break;
                case TOK_GT:
                    node->type = TYPE_INT;
                    break;
                case TOK_LTE:
                    node->type = TYPE_INT;
                    break;
                case TOK_GTE:
                    node->type = TYPE_INT;
                    break;
                case TOK_EQEQ:
                    node->type = TYPE_INT;
                    break;
                case TOK_NEQ:
                    node->type = TYPE_INT;
                    break;
                case TOK_AND:
                    node->type = TYPE_INT;
                    break;
                case TOK_OR:
                    node->type = TYPE_INT;
                    break;
                default:
                    node->type = TYPE_INT;
                    break;
            }
            break;
        case NODE_UNOP : 
            sema_node(sema, node->left);

            //determine type based on operator and operand type
            switch(node->token.kind) {
                case TOK_MINUS :
                    node->type = node->left->type;
                    break;
                case TOK_NOT :
                    node->type = TYPE_INT;
                    break;
                case TOK_PLUSPLUS :
                    node->type = node->left->type;
                    break;
                case TOK_MINUSMINUS :
                    node->type = node->left->type;
                    break;
                //return type of dereference
                case TOK_STAR :
                    switch(node->left->type) {
                        case TYPE_INT_PTR :
                            node->type = TYPE_INT;
                            break;
                        case TYPE_CHAR_PTR :
                            node->type = TYPE_CHAR;
                            break;
                        default :
                            error(node->token.line, "dereferencing non-pointer type");
                            break;
                    }
                    break;
                //return type of reference
                case TOK_AMPERSAND :
                     switch(node->left->type) {
                        case TYPE_INT :
                            node->type = TYPE_INT_PTR;
                            break;
                        case TYPE_CHAR :
                            node->type = TYPE_CHAR_PTR;
                            break;
                        default :
                            error(node->token.line, "taking address of non-scalar type");
                            break;
                    }
                    break;
                default :
                    break;
            }
            break;
        case NODE_CALL : {
            FuncSymbol* func_symbol = lookup_function(sema, node->token.start, node->token.length);
            if(func_symbol == NULL) {
                error(node->token.line, "undeclared function '%.*s'", node->token.length, node->token.start);
            }

            //walk each argument and check type against parameter type
            int i = 0;
            ASTNode* node_arg = node->left;
            while(node_arg != NULL) {
                //crucial, walk arg before checking
                sema_node(sema, node_arg);

                //if non-variadic function has more arguments than the function has parameters
                if(func_symbol->param_count != -1 && i >= func_symbol->param_count) {
                    error(node->token.line, "too many arguments to function '%.*s'", func_symbol->length, func_symbol->start);
                }

                TypeKind param_type = func_symbol->param_types[i];
                TypeKind arg_type = node_arg->type;

                //check types of this arg with that of corresponding function parameter
                if(func_symbol->param_count != -1 && !types_compatible(param_type, arg_type)) {
                    error(node->token.line, "incompatible type for argument %d of '%.*s'", i+1, func_symbol->length, func_symbol->start);
                }

                //move on to next arg
                node_arg = node_arg->next;
                i++;
            }

            //if num of args and num of parameters dont match. skip variadic functions (functions that param_count == -1)
            if(func_symbol->param_count != -1 && i < func_symbol->param_count) {
                error(node->token.line, "too few arguments to function '%.*s', expected %d but got %d", func_symbol->length, func_symbol->start, func_symbol->param_count, i);
            }

            //annotate node type with function return type
            node->type = func_symbol->return_type;
            break;
        }
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
            node->type = node->left->type;
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
            break;
    }
}
