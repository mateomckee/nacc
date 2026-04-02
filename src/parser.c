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

    //proceeed to consume and return
    advance(parser);
    return parser->current_token;
}


//all parse functions below, these represent a production in my CFG
//these functions call each other recursively until all tokens are parsed

//entry point of parser, begins parsing, starting from the root node downwards
ASTNode* parse_program(Parser* parser) {
    ASTNode* root;
    
    print_token(parser->previous_token);
    print_token(parser->current_token);
    advance(parser);  

    return root;
}

//top level productions
//program
//function
//block
