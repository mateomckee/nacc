#include "lexer.h"

// NODE_FUNC:   left=params  right=body
// NODE_IF:     left=cond    right=then   extra1=else
// NODE_WHILE:  left=cond    right=body
// NODE_FOR:    left=init    right=body   extra1=cond  extra2=post
// NODE_DECL:   left=init    right=NULL
// NODE_RETURN: left=expr    right=NULL
// NODE_BINOP:  left=left    right=right
// NODE_UNOP:   left=operand
// NODE_CALL:   left=args


typedef struct {
    Lexer* lexer;
    Token current_token;
    Token previous_token;
} Parser;

typedef enum {
    NODE_PROGRAM,
    NODE_FUNC,
    NODE_PARAM,
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

typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_INT_PTR,
    TYPE_CHAR_PTR
} TypeKind;

//tree structure
typedef struct {
    NodeKind kind;
    TypeKind type; //used only for nodes with a type (NODE_FUNC, NODE_DECL, NODE_PARAM), otherwise goes unused, will be helpful later in sema.c when checking types

    Token token;

    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* extra1; //extra child node in case of if/else, for loop parsing
    struct ASTNode* extra2; //specifically for for loops (initialization; body; condition; increment)
} ASTNode;

void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
