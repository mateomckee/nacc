#include <stdio.h>
#include "parser.h"
#include "util.h"

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
    node->extra1 = NULL;
    node->extra2 = NULL;
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

ASTNode* parse_primary() {
    ASTNode* node;

    return node;
}

ASTNode* parse_block(Parser* parser) {
    ASTNode* node;
    
    return node;
}

ASTNode* parse_params(Parser* parser) {
    ASTNode* node;

    //parse_params is only called when there must be params inside a function declaration
    

    
    return node;
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
    ASTNode* root;

    //if first token is a type or void, parse for a function, check does not consume token just reads, parse_function will consume expecting a full function definition
    while(check(parser, TOK_INT) || check(parser, TOK_CHAR) || check(parser, TOK_VOID)) {
        ASTNode* node = parse_function(parser);

        
    }

    return root;
}

