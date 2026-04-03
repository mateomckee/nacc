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
    NODE_POSTFIX,

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
    struct ASTNode* extra; 
    struct ASTNode* extra2;
    struct ASTNode* next;   //additional 4th child for "next sibling" pointing, also used in for loops
} ASTNode;

void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void print_ast(ASTNode* node, int depth);
