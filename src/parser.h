#include "lexer.h"

typedef struct {
    Lexer* lexer;
    Token current_token;
    Token previous_token;
} Parser;

typedef enum {
    NODE_PROGRAM,
    NODE_FUNC,
    NODE_BLOCK,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_DECL,
    NODE_ASSIGN,
    NODE_BINOP,
    NODE_UNOP,
    NODE_CALL,
    NODE_IDENT,

    NODE_INT_LIT,
    NODE_CHAR_LIT,
    NODE_STRING_LIT
} NodeKind;

//tree structure
typedef struct {
    NodeKind kind;
    Token token;

    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* extra; //extra child node in case of if/else parsing
} ASTNode;

void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
