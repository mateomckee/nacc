typedef enum {
   // tokens are organized into groups that make sense together for readability, but understand a single token can mean several things
   
   // any ID like variables, functions, etc. 
   TOK_ID,

   // literals
   TOK_INT_LIT,
   TOK_CHAR_LIT,
   TOK_STRING_LIT,

   // arithmetic operators
   TOK_PLUS,
   TOK_MINUS,
   TOK_STAR,    // also used as pointer/dereference
   TOK_SLASH,
   TOK_EQUAL,
    
   TOK_PLUSEQ,
   TOK_MINUSEQ,
   TOK_MULTEQ,
   TOK_DIVEQ,

   TOK_AMPERSAND, // &
 
   // increment/decrement
   TOK_PLUSPLUS,
   TOK_MINUSMINUS,

   // logical operators
   TOK_AND,     // &&
   TOK_OR,      // ||
   TOK_NOT,     // !

   // comparison operators
   TOK_EQEQ,    // ==
   TOK_NEQ,     // !=
   TOK_LT,      // <
   TOK_GT,      // >
   TOK_LTE,     // <=
   TOK_GTE,     // >=

   // () [] {}
   TOK_LPAREN,
   TOK_RPAREN,
   TOK_LBRACKET,
   TOK_RBRACKET,
   TOK_LBRACE,
   TOK_RBRACE,

   // key words - control flow
   TOK_IF,
   TOK_ELSE,
   TOK_WHILE,
   TOK_FOR,

   //key words - functions
   TOK_RETURN,
   TOK_VOID,

   // types
   TOK_INT,
   TOK_CHAR,
   
   // special chars
   TOK_COMMA,
   TOK_SEMICOLON,
   TOK_EOF,
   TOK_ERROR
} TokenKind;

typedef struct {
    TokenKind kind;
    const char* start;
    int length;
    int line; //line number for error printing
} Token;

typedef struct {
    const char* source;
    int pos;
    int line;
} Lexer;

void lexer_init(Lexer* lexer, char* source);

char advance(Lexer* lexer);

char peek(Lexer* lexer);

void skip_ws(Lexer* lexer);

Token next_token(Lexer* lexer);

const char* token_kind_str(TokenKind kind);

void print_token(Token token);
