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
    TYPE_CHAR_PTR,
    TYPE_VOID_PTR
} TypeKind;

//tree structure
typedef struct ASTNode {
    NodeKind kind;
    TypeKind type; //used only for nodes that hold a type (NODE_FUNC, NODE_DECL, NODE_PARAM), otherwise goes unused, will be helpful later in sema.c when checking types, and implementing the symbol table

    Token token;

    struct ASTNode* left;
    struct ASTNode* right;
    //extra child nodes for productions that store additional data such as if and for statements
    struct ASTNode* extra; 
    struct ASTNode* extra2;
    
    struct ASTNode* next;   // special child node reserved for storing "sibling" nodes as a linked list
                            // for example, a NODE_FUNC node is stored as the left child of NODE_PROGRAM, it is the head of the linked list and contains the rest of the programs NODE_FUNC nodes through its "next" value
                            // same pattern for storing params, and args
} ASTNode;

void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void print_ast(ASTNode* node, int depth);
const char* node_kind_str(NodeKind kind);
