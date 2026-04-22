#include <stdio.h>
#include "parser.h"
#include "util.h"

//forward declarations
ASTNode* parse_program(Parser* parser);
ASTNode* parse_function(Parser* parser);
ASTNode* parse_params(Parser* parser);
ASTNode* parse_block(Parser* parser);
ASTNode* parse_stmt(Parser* parser);
ASTNode* parse_if(Parser* parser);
ASTNode* parse_while(Parser* parser);
ASTNode* parse_for(Parser* parser);
ASTNode* parse_return(Parser* parser);
ASTNode* parse_decl(Parser* parser);
ASTNode* parse_expr_stmt(Parser* parser);
ASTNode* parse_expr(Parser* parser);
ASTNode* parse_assign(Parser* parser);
ASTNode* parse_or(Parser* parser);
ASTNode* parse_and(Parser* parser);
ASTNode* parse_equality(Parser* parser);
ASTNode* parse_compare(Parser* parser);
ASTNode* parse_add(Parser* parser);
ASTNode* parse_mul(Parser* parser);
ASTNode* parse_unary(Parser* parser);
ASTNode* parse_primary(Parser* parser);
ASTNode* parse_args(Parser* parser);

//core functions of the parser, each production function uses these to continue building tree
void parser_init(Parser* parser, Lexer* lexer) {
    //init the parser
    //load in lexer
    parser->lexer = lexer;
    parser->current_token = next_token(parser->lexer); // prime parser with first token
}

void advance(Parser* parser) {
    //move forward
    //saves current into previous
    parser->previous_token = parser->current_token;
    //call next_token to get the next token into current
    parser->current_token = next_token(parser->lexer);
}

int check(Parser* parser, TokenKind kind) {
    //look at current.kind and return true or false, dont consume
    return parser->current_token.kind == kind ? 1 : 0;
}

int match(Parser* parser, TokenKind kind) {
    //for optional tokens
    //call check, if pass, advance, return true
    if(check(parser, kind)) { advance(parser); return 1;}
    return 0;
}

Token expect(Parser* parser, TokenKind kind, const char* msg) {
    //use when token is required, like a ;
    //call match, if fails throw error
    if(!match(parser, kind)) { error(parser->current_token.line, msg); }
    return parser->previous_token;
}

//ASTNode* helper functions to make node creation easier 
//generic make_node function
//make_binop and make_unop node functions for most common cases

ASTNode* make_node(NodeKind kind, Token token) {
    ASTNode* node = nacc_malloc(sizeof(ASTNode));
    node->kind = kind;
    node->token = token;
    node->type = TYPE_NONE;
    node->left = NULL;
    node->right = NULL;
    node->extra = NULL;
    node->extra2 = NULL;
    node->next = NULL;
    return node;
}

ASTNode* make_binop(Token op, ASTNode* left, ASTNode* right) {
    ASTNode* node = make_node(NODE_BINOP, op);
    node->left = left;
    node->right = right;
    return node;
}
ASTNode* make_unop(Token op, ASTNode* operand) {
    ASTNode* node = make_node(NODE_UNOP, op);
    node->left = operand;
    return node;
}

//helper function to consume a type token and return it
TypeKind consume_type(Parser* parser) {
    TypeKind type;

    //check for which type token we're at
    switch(parser->current_token.kind) {
        case TOK_INT :  
            advance(parser);
            //check for pointer declaration by matching next token
            type = (match(parser, TOK_STAR)) ? TYPE_INT_PTR : TYPE_INT;
            break;
        case TOK_CHAR :
            advance(parser);
            type = (match(parser, TOK_STAR)) ? TYPE_CHAR_PTR : TYPE_CHAR;
            break;
        case TOK_VOID :
            advance(parser);
            type = TYPE_VOID;
            break;
        default :
            error(parser->current_token.line, "parsed token is not a type");
    }
    return type;
}

//all parse functions below, these represent a production in my CFG
//these functions call each other recursively until all tokens are parsed
// * or + in the CFG (regex repeat) means the parse function should continue looping while the matches are valid, otherwise just check once

//entry point of parser, begins parsing, starting from the root node downwards
//parse_program -> function*

//im implementing first the productions that depend on the least amount of other productions, bottom-up, better for testing
//and because top level functions require calls to lower level functions, so low level functions must be declared first

ASTNode* parse_args(Parser* parser) {
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    while(!check(parser, TOK_RPAREN)) {
        if(head != NULL) {
            expect(parser, TOK_COMMA, "expected \',\' after argument");
        }
        ASTNode* expr = parse_expr(parser);

        if(head == NULL) {
            head = expr;
            tail = expr;
        }
        else {
            tail->next = expr;
            tail = tail->next;
        }
    }

    return head;
}

//base case
ASTNode* parse_primary(Parser* parser) {
    Token token = parser->current_token;
    TokenKind kind = token.kind;
    advance(parser);

    switch(kind) {
        case TOK_INT_LIT :
            return make_node(NODE_INT_LIT, token);
        case TOK_CHAR_LIT :
            return make_node(NODE_CHAR_LIT, token);
        case TOK_STRING_LIT :
            return make_node(NODE_STRING_LIT, token);
        case TOK_LPAREN : {
            // ( expr )
            ASTNode* expr = parse_expr(parser);
            expect(parser, TOK_RPAREN, "expected \')\' after expression");
            return expr;
        }
        case TOK_ID : {
            // ID( is a function call
            ASTNode* ident = make_node(NODE_IDENT, token);
            if(check(parser, TOK_LPAREN)) {
                advance(parser); //consume '('
                ASTNode* args = parse_args(parser);
                expect(parser, TOK_RPAREN, "expected \')\' after parameters");
                ASTNode* call = make_node(NODE_CALL, token);
                call->left = args;
                return call;
            }
            else if(check(parser, TOK_PLUSPLUS) || check(parser, TOK_MINUSMINUS)) {
                advance(parser);
                Token operator = parser->previous_token;
                ASTNode* post = make_node(NODE_POSTFIX, operator);
                post->left = ident;
                return post;
            }
            return ident;
        }
        default :
            error(token.line, "expected expression");
            return NULL;
    }
}

ASTNode* parse_unary(Parser* parser) {
    ASTNode* right = NULL;
    if(check(parser, TOK_PLUSPLUS) || check(parser, TOK_MINUSMINUS) || check(parser, TOK_MINUS) || check(parser, TOK_NOT) || check(parser, TOK_STAR) || check(parser, TOK_AMPERSAND)) {
        Token operator = parser->current_token;
        advance(parser);
        right = parse_unary(parser);
        return make_unop(operator, right);
    }
    else {
        return parse_primary(parser);
    }
}

ASTNode* parse_mul(Parser* parser) {
    ASTNode* left = parse_unary(parser);
    ASTNode* right = NULL;

    while(check(parser, TOK_STAR) || check(parser, TOK_SLASH)) {
        match(parser, TOK_STAR) || match(parser, TOK_SLASH);

        Token operator = parser->previous_token;
        right = parse_unary(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}


ASTNode* parse_add(Parser* parser) {
    ASTNode* left = parse_mul(parser);
    ASTNode* right = NULL;

    while(check(parser, TOK_PLUS) || check(parser, TOK_MINUS)) {
        match(parser, TOK_PLUS) || match(parser, TOK_MINUS);

        Token operator = parser->previous_token;
        right = parse_mul(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}

ASTNode* parse_compare(Parser* parser) {
    ASTNode* left = parse_add(parser);
    ASTNode* right = NULL;

    while(check(parser, TOK_LT) || check(parser, TOK_GT) || check(parser, TOK_LTE) || check(parser, TOK_GTE)) {
        match(parser, TOK_LT) || match(parser, TOK_GT) || match(parser, TOK_LTE) || match(parser, TOK_GTE);

        Token operator = parser->previous_token;
        right = parse_add(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}

ASTNode* parse_equality(Parser* parser) {
    ASTNode* left = parse_compare(parser);
    ASTNode* right = NULL;

    while(check(parser, TOK_EQEQ) || check(parser, TOK_NEQ)) {
        match(parser, TOK_EQEQ) || match(parser, TOK_NEQ);

        Token operator = parser->previous_token;
        right = parse_compare(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}

ASTNode* parse_and(Parser* parser) {
    ASTNode* left = parse_equality(parser);
    ASTNode* right = NULL;

    while(match(parser, TOK_AND)) {
        Token operator = parser->previous_token;
        right = parse_equality(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}


ASTNode* parse_or(Parser* parser) {
    ASTNode* left = parse_and(parser);
    ASTNode* right = NULL;

    while(match(parser, TOK_OR)) {
        Token operator = parser->previous_token;
        right = parse_and(parser);
        left = make_binop(operator, left, right); //left becomes new root
    }
    return left;
}

ASTNode* parse_assign(Parser* parser) {
    //calls down the precedence chain
    ASTNode* left = parse_or(parser);

    if(!check(parser, TOK_EQUAL) && !check(parser, TOK_PLUSEQ) && !check(parser, TOK_MINUSEQ) && !check(parser, TOK_MULTEQ) && !check(parser, TOK_DIVEQ)) {
        return left;
    }

    // consume whichever assignment operator it is
    match(parser, TOK_EQUAL) || match(parser, TOK_PLUSEQ) || 
    match(parser, TOK_MINUSEQ) || match(parser, TOK_MULTEQ) || 
    match(parser, TOK_DIVEQ);
    
    Token operator = parser->previous_token;
    ASTNode* right = parse_assign(parser); //recursive call is right-associative, which is what we want for assignment

    ASTNode* node = make_node(NODE_ASSIGN, operator); //must store previous token on assignment
    node->left = left;
    node->right = right;
    return node;
}

//entry point into chain of precedence for things like assignment, logical comparison, arithmetic, unary, primary, etc.
ASTNode* parse_expr(Parser* parser) {
    return parse_assign(parser);
}

ASTNode* parse_expr_stmt(Parser* parser) {
    ASTNode* node = parse_expr(parser);
    expect(parser, TOK_SEMICOLON, "expected \';\' after expression");
    return node;
}

ASTNode* parse_decl(Parser* parser) {
    TypeKind type = consume_type(parser);

    Token token = expect(parser, TOK_ID, "expected variable name");

    ASTNode* left = NULL;
    //if this declaration is also initializing
    if(match(parser, TOK_EQUAL)) {
        left = parse_expr(parser);
    }
    
    expect(parser, TOK_SEMICOLON, "expected \';\' after declaration");

    ASTNode* node = make_node(NODE_DECL, token);
    node->type = type;
    node->token = token;
    node->left = left;

    return node;
}

ASTNode* parse_return(Parser* parser) {
    Token token = expect(parser, TOK_RETURN, "expected return statement");

    ASTNode* left = NULL;
    if(!check(parser, TOK_SEMICOLON)) {
        left = parse_expr(parser);
    }

    expect(parser, TOK_SEMICOLON, "expected \';\' after return statement");

    ASTNode* node = make_node(NODE_RETURN, token);
    node->left = left;

    return node;
}

ASTNode* parse_for(Parser* parser) {
    Token token = expect(parser, TOK_FOR, "expected for statement");

    expect(parser, TOK_LPAREN, "expected \'(\' after for statement");

    ASTNode* left = NULL;
    if (check(parser, TOK_INT) || check(parser, TOK_CHAR)) { 
        left = parse_decl(parser); //semicolon included
    }
    else {
        left = parse_assign(parser);
        expect(parser, TOK_SEMICOLON, "expected \';\' after for assignment");
    }

    ASTNode* right = parse_expr(parser);

    expect(parser, TOK_SEMICOLON, "expected \';\' after for condition");

    ASTNode* extra2 = parse_expr(parser);

    expect(parser, TOK_RPAREN, "expected \')\' after for post");

    ASTNode* extra = parse_block(parser);

    ASTNode* node = make_node(NODE_FOR, token);
    node->left = left;
    node->right = right;
    node->extra = extra;
    node->extra2 = extra2;

    return node;
}

ASTNode* parse_while(Parser* parser) {
    Token token = expect(parser, TOK_WHILE, "expected while statement");

    expect(parser, TOK_LPAREN, "expected \'(\' after while statement");

    ASTNode* left = parse_expr(parser);

    expect(parser, TOK_RPAREN, "expected \')\' after while condition");

    ASTNode* right = parse_block(parser);


    ASTNode* node = make_node(NODE_WHILE, token);
    node->left = left;
    node->right = right;

    return node;
}

ASTNode* parse_if(Parser* parser) {
    Token token = expect(parser, TOK_IF, "expected if statement");

    expect(parser, TOK_LPAREN, "expected \'(\' after if statement");

    ASTNode* left = parse_expr(parser);

    expect(parser, TOK_RPAREN, "expected\')\' after if statement");

    ASTNode* right = parse_block(parser);

    ASTNode* extra = NULL;
    if(match(parser, TOK_ELSE)) {
        if(check(parser, TOK_IF)) {
            extra = parse_if(parser);
        }
        else {
            extra = parse_block(parser);
        }
    }

    ASTNode* node = make_node(NODE_IF, token);
    node->left = left;
    node->right = right;
    node->extra = extra;

    return node;
}

//parse_statement is a dispatcher that routes to the correct type of statement based on token
//statements are complete units of executions, end in ; or a block { }
ASTNode* parse_statement(Parser* parser) {
    switch(parser->current_token.kind) {
        case TOK_IF :
            return parse_if(parser);
        case TOK_WHILE :
            return parse_while(parser);
        case TOK_FOR :
            return parse_for(parser);
        case TOK_RETURN :
            return parse_return(parser);
        //3 cases below all fall into declaration
        case TOK_INT :
        case TOK_CHAR :
        case TOK_VOID :
            return parse_decl(parser);
        default :
            return parse_expr_stmt(parser);
    }
}

ASTNode* parse_block(Parser* parser) {
    ASTNode* node = make_node(NODE_BLOCK, parser->current_token);

    expect(parser, TOK_LBRACE, "expected \'{\' before statements");

    //parse all statements until block closes
    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    while(!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if(head == NULL) {
            head = stmt;
            tail = stmt;
        }
        else {
            tail->next = stmt;
            tail = tail->next;
        }
    }

    expect(parser, TOK_RBRACE, "expected \'}\' afer statements");

    node->left = head;

    return node;
}

ASTNode* parse_params(Parser* parser) {
    ASTNode* head = NULL;
    ASTNode* tail = NULL;

    //while current token is not a right parenthesis, continue parsing parameters
    while(!check(parser, TOK_RPAREN)) {
        //if not first param and next token is not ')', expect a comma
        if(head != NULL) {
            expect(parser, TOK_COMMA, "expected \',\' after parameter name");
        }

        //parse parameter
        TypeKind type = consume_type(parser);
        Token token = expect(parser, TOK_ID, "expected parameter name");

        //create parameter node
        ASTNode* node = make_node(NODE_PARAM, token);
        node->type = type;
        node->token = token;

        if(head == NULL) {
            head = node;
            tail = node;
        }
        else {
            tail->next = node;
            tail = tail->next;
        }
    }
    
    return head;
}

ASTNode* parse_function(Parser* parser) {
    //production rules
    TypeKind type = consume_type(parser);

    Token token = expect(parser, TOK_ID, "expected function name");

    expect(parser, TOK_LPAREN, "expected \'(\' after function name");

    ASTNode* left = NULL;
    //if no right paren after left paren, we have parameters and must parse them
    if(!check(parser, TOK_RPAREN)) {
        left = parse_params(parser);
    }

    expect(parser, TOK_RPAREN, "expected \')\' after parameters");

    ASTNode* right = parse_block(parser);

    //build this node from parsed information
    ASTNode* node = make_node(NODE_FUNC, token);
    node->type = type;
    node->left = left;
    node->right = right;

    return node;
}

//make each expect, match, check call per line. if something is expected and not found, prints error and exits program. if found, advance to next token
//if something is matched and not found, ignore because its optional. if found, advance to next token

ASTNode* parse_program(Parser* parser) {
    //if first token is a type or void, parse for a function, check does not consume token just reads, parse_function will consume expecting a full function definition
    //since ASTNode has limited child nodes, I'm going to use a linked-list approach to store functions as they are parsed. the head will be the left node of NODE_PROGRAM node, and from there all remaining functions are stored
    //in the next pointer (every node has 2 children, 1 extra, 1 next), forming a linked-list

    //token passed in is not meaningful, just needed to create node
    ASTNode* root = make_node(NODE_PROGRAM, parser->current_token);
   
    int i = 0;

    ASTNode* head = NULL;
    ASTNode* tail = NULL;
    while(check(parser, TOK_INT) || check(parser, TOK_CHAR) || check(parser, TOK_VOID)) {
        ASTNode* func = parse_function(parser);
        if(head == NULL) {
            head = func;
            tail = func;
        }
        else {
            tail->next = func;
            tail = tail->next;
        }
        i++;
    }

    expect(parser, TOK_EOF, "unexpected token at top level");
    
    root->left = head;

    return root;
}

//AI generated helper functions to print/visualize my AST
//not important to the compilation process, just to have something to show my professor visually
const char* node_kind_str(NodeKind kind) {
    switch(kind) {
        case NODE_PROGRAM:    return "NODE_PROGRAM";
        case NODE_FUNC:       return "NODE_FUNC";
        case NODE_BLOCK:      return "NODE_BLOCK";
        case NODE_PARAM:      return "NODE_PARAM";
        case NODE_IF:         return "NODE_IF";
        case NODE_WHILE:      return "NODE_WHILE";
        case NODE_FOR:        return "NODE_FOR";
        case NODE_RETURN:     return "NODE_RETURN";
        case NODE_DECL:       return "NODE_DECL";
        case NODE_ASSIGN:     return "NODE_ASSIGN";
        case NODE_BINOP:      return "NODE_BINOP";
        case NODE_UNOP:       return "NODE_UNOP";
        case NODE_CALL:       return "NODE_CALL";
        case NODE_IDENT:      return "NODE_IDENT";
        case NODE_POSTFIX:    return "NODE_POSTFIX";
        case NODE_INT_LIT:    return "NODE_INT_LIT";
        case NODE_CHAR_LIT:   return "NODE_CHAR_LIT";
        case NODE_STRING_LIT: return "NODE_STRING_LIT";
        default:              return "UNKNOWN";
    }
}

const char* type_kind_str(TypeKind kind) {
    switch(kind) {
        case TYPE_NONE:     return "none";
        case TYPE_INT:      return "int";
        case TYPE_CHAR:     return "char";
        case TYPE_VOID:     return "void";
        case TYPE_INT_PTR:  return "int*";
        case TYPE_CHAR_PTR: return "char*";
        default:            return "unknown";
    }
}

void print_ast(ASTNode* node, int depth) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("  ");

    printf("[%s]", node_kind_str(node->kind));
    if (node->token.length > 0) {
        printf(" '%.*s'", node->token.length, node->token.start);
    }
    if (node->type != TYPE_NONE) {
        printf(" (%s)", type_kind_str(node->type));
    }
    printf("\n");

    print_ast(node->left,   depth + 1);
    print_ast(node->right,  depth + 1);

    if (node->kind == NODE_FOR) {
        print_ast(node->extra2, depth + 1);  // post expression
        print_ast(node->extra,  depth + 1);  // body block
    } else {
        print_ast(node->extra,  depth + 1);  // else branch etc
    }

    print_ast(node->next, depth);  // sibling, always same depth
}
