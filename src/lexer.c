#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"
#include "util.h"

//helper methods to make creating tokens cleaner and quicker
Token make_token(TokenKind kind, const char* start, int length, int line) {
    Token t;
    t.kind = kind;
    t.start = start;
    t.length = length;
    t.line = line;
    return t;
}
Token make_single(TokenKind kind, Lexer* lexer) {
    return make_token(kind, lexer->source+lexer->pos - 1, 1, lexer->line);
}
Token make_error(Lexer* lexer, const char* msg) {
    fprintf(stderr, "lexer error on line %d: %s\n", lexer->line, msg);
    return make_single(TOK_ERROR, lexer);
}

// external function used to initialize Lexer struct
void lexer_init(Lexer* lexer, char* source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
}

// advance to next char and return it
static char advance(Lexer* lexer) {
    return (lexer->source)[lexer->pos++];
}

// return current char
char peek(Lexer* lexer) {
    return (lexer->source)[lexer->pos];
}

void skip_single_line_comment(Lexer* lexer) {
    char at = peek(lexer);
    while(at != '\n' && at != '\0') {
        advance(lexer);
        at = peek(lexer);
    }
}

void skip_multi_line_comment(Lexer* lexer) {
    advance(lexer); //consume * so it doesn't confuse it as the end of comment
    while(1) {
        if(peek(lexer) == '\0') {
            error(lexer->line, "unterminated block comment");
        }
        if(peek(lexer) == '\n') lexer->line++;

        char c = advance(lexer);
        if(c == '*' && peek(lexer) == '/') {
            advance(lexer); //consume closing /
            break;
        }
    }
}

// continuously skips chars if they are whitespace
void skip_ws(Lexer* lexer) {
    char at = peek(lexer);
    while(at == ' ' || at == '\t' || at == '\r' || at == '\n') {
        if(at == '\n') lexer->line++;
        advance(lexer);
        at = peek(lexer);
    }
}

Token scan_int_literal(Lexer* lexer) {
    int start = lexer->pos-1;
    int len = 1;
    while(isdigit(peek(lexer))) { advance(lexer); len++; }

    return make_token(TOK_INT_LIT, lexer->source+start, len, lexer->line);
}

//scans only the contents of the string, not the quotes
Token scan_string_literal(Lexer* lexer) {
    int start = lexer->pos;
    int len = 0;
    char at;
    while((at = advance(lexer)) != '"' && at != '\0') { 
        if(at == '\\') { advance(lexer); len++; } //escape sequence
        len++;
    }

    return make_token(TOK_STRING_LIT, lexer->source+start, len, lexer->line);
}

Token scan_char_literal(Lexer* lexer) {
    int start = lexer->pos;
    int len = 0; 

    //grab content char
    char at = advance(lexer);

    //if no content char (empty literal or EOF), return error
    if(at == '\'') {
        return make_error(lexer, "empty char literal");
    }
    else if(at == '\0') {
        return make_error(lexer, "unclosed char literal");
    }
    //grabbed content char, now check for escape sequence and close
    else {
        if(at == '\\') {
            advance(lexer);
            len++;
        }

        len++;

        //check if char literal is closed properly
        at = advance(lexer);
        if(at != '\'') {
            return make_error(lexer, "unclosed char literal");
        }
    }

    return make_token(TOK_CHAR_LIT, lexer->source+start, len, lexer->line);
}

Token scan_identifier(Lexer* lexer) {
    int start = lexer->pos-1;
    int len = 1;
    char at;
    while(isalnum(at = peek(lexer)) || at == '_') { advance(lexer); len++; }

    //check for keywords and return any matches
    if (strncmp(lexer->source + start, "int", len) == 0 && len == 3)
        return make_token(TOK_INT, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "char", len) == 0 && len == 4)
        return make_token(TOK_CHAR, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "if", len) == 0 && len == 2)
        return make_token(TOK_IF, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "else", len) == 0 && len == 4)
        return make_token(TOK_ELSE, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "while", len) == 0 && len == 5)
        return make_token(TOK_WHILE, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "for", len) == 0 && len == 3)
        return make_token(TOK_FOR, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "return", len) == 0 && len == 6)
        return make_token(TOK_RETURN, lexer->source + start, len, lexer->line);
    if (strncmp(lexer->source + start, "void", len) == 0 && len == 4)
        return make_token(TOK_VOID, lexer->source + start, len, lexer->line);

    //return just an ID
    return make_token(TOK_ID, lexer->source+start, len, lexer->line);
}

// external function that drives lexer
Token next_token(Lexer* lexer) {
    //skip whitespace until we reach a character
    skip_ws(lexer);

    //read char
    char start_char = advance(lexer);

    //EOF
    if(start_char == '\0') {
        return make_token(TOK_EOF, lexer->source+lexer->pos, 0, lexer->line);
    }

    //int literal
    else if(isdigit(start_char)) {
        return scan_int_literal(lexer);
    }
    //string literal
    else if(start_char == '\"') {
        return scan_string_literal(lexer);
    }
    //char literal
    else if(start_char == '\'') {
        return scan_char_literal(lexer);
    }

    //identifier
    else if(isalpha(start_char) || start_char == '_') {
        return scan_identifier(lexer);
    }
    //anything else (punctuation, operators), use a switch
    else {
        switch(start_char) {
            case '+' :
                if (peek(lexer) == '+') {
                    advance(lexer);
                    return make_token(TOK_PLUSPLUS, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_PLUSEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_PLUS, lexer);

            case '-' :
                 if (peek(lexer) == '-') {
                    advance(lexer);
                    return make_token(TOK_MINUSMINUS, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_MINUSEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_MINUS, lexer);

            case '*' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_MULTEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_STAR, lexer);

            case '/' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_DIVEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                //single line comment
                if(peek(lexer) == '/') {
                    skip_single_line_comment(lexer);
                    return next_token(lexer);
                }
                //multi line comment
                if(peek(lexer) == '*') {
                    skip_multi_line_comment(lexer);
                    return next_token(lexer);
                }
                return make_single(TOK_SLASH, lexer);

            case '=' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_EQEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_EQUAL, lexer);

            case '&' :
                if (peek(lexer) == '&') {
                    advance(lexer);
                    return make_token(TOK_AND, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_AMPERSAND, lexer);

            case '!' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_NEQ, lexer->source+lexer->pos-2, 2, lexer->line);
                }

                return make_single(TOK_NOT, lexer);

            case '|' :
                if (peek(lexer) == '|') {
                    advance(lexer);
                    return make_token(TOK_OR, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                //no bitwise operator
                break;

            case '<' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_LTE, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_LT, lexer);

            case '>' :
                if (peek(lexer) == '=') {
                    advance(lexer);
                    return make_token(TOK_GTE, lexer->source+lexer->pos-2, 2, lexer->line);
                }
                return make_single(TOK_GT, lexer);


            case '(' :
                return make_single(TOK_LPAREN, lexer);
                break;

            case ')' :
                return make_single(TOK_RPAREN, lexer);
                break;
                
            case '[' :
                return make_single(TOK_LBRACKET, lexer);
                break;

            case ']' :
                return make_single(TOK_RBRACKET, lexer);
                break;

            case '{' :
                return make_single(TOK_LBRACE, lexer);
                break;

            case '}' :
                return make_single(TOK_RBRACE, lexer);
                break;

            case ',' :
                return make_single(TOK_COMMA, lexer); 
                break;

            case ';' :
                return make_single(TOK_SEMICOLON, lexer);
                break;
            
            default:
                break;
        }
    }

    return make_error(lexer, "unexpected character");
}

//helper functions

void print_token(Token t) {
    printf("Token { kind: %s, value: '%.*s', line: %d }\n",
        token_kind_str(t.kind), t.length, t.start, t.line);
}

const char* token_kind_str(TokenKind kind) {
    switch(kind) {
        case TOK_ID:          return "TOK_ID";
        case TOK_INT_LIT:     return "TOK_INT_LIT";
        case TOK_CHAR_LIT:    return "TOK_CHAR_LIT";
        case TOK_STRING_LIT:  return "TOK_STRING_LIT";
        case TOK_PLUS:        return "TOK_PLUS";
        case TOK_MINUS:       return "TOK_MINUS";
        case TOK_STAR:        return "TOK_STAR";
        case TOK_SLASH:       return "TOK_SLASH";
        case TOK_EQUAL:       return "TOK_EQUAL";
        case TOK_AMPERSAND:   return "TOK_AMPERSAND";
        case TOK_PLUSPLUS:    return "TOK_PLUSPLUS";
        case TOK_MINUSMINUS:  return "TOK_MINUSMINUS";
        case TOK_AND:         return "TOK_AND";
        case TOK_OR:          return "TOK_OR";
        case TOK_NOT:         return "TOK_NOT";
        case TOK_EQEQ:        return "TOK_EQEQ";
        case TOK_NEQ:         return "TOK_NEQ";
        case TOK_LT:          return "TOK_LT";
        case TOK_GT:          return "TOK_GT";
        case TOK_LTE:         return "TOK_LTE";
        case TOK_GTE:         return "TOK_GTE";
        case TOK_LPAREN:      return "TOK_LPAREN";
        case TOK_RPAREN:      return "TOK_RPAREN";
        case TOK_LBRACKET:    return "TOK_LBRACKET";
        case TOK_RBRACKET:    return "TOK_RBRACKET";
        case TOK_LBRACE:      return "TOK_LBRACE";
        case TOK_RBRACE:      return "TOK_RBRACE";
        case TOK_IF:          return "TOK_IF";
        case TOK_ELSE:        return "TOK_ELSE";
        case TOK_WHILE:       return "TOK_WHILE";
        case TOK_FOR:         return "TOK_FOR";
        case TOK_RETURN:      return "TOK_RETURN";
        case TOK_VOID:        return "TOK_VOID";
        case TOK_INT:         return "TOK_INT";
        case TOK_CHAR:        return "TOK_CHAR";
        case TOK_COMMA:       return "TOK_COMMA";
        case TOK_SEMICOLON:   return "TOK_SEMICOLON";
        case TOK_EOF:         return "TOK_EOF";
        default:              return "UNKNOWN";
    }
}
