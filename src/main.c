#include <stdio.h>
#include <stdlib.h>
#include "tac.h"

//simple function to take file contents and return them as a string
char* get_file_as_string(const char* filepath) {
    FILE* f = fopen(filepath, "r");

    if (!f) {
        fprintf(stderr, "could not open file %s\n", filepath);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* output = malloc(size + 1);
    fread(output, 1, size, f);
    output[size] = '\0'; //null-terminate

    fclose(f);
    return output;
}

// pass in the filepath of the source program
//  ../tests/count.c
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Please provide the filepath of the source code\n");
        return 1;
    }

    //get source program as a character stream
    const char* filepath = argv[1];
    char* char_stream = get_file_as_string(filepath);

    //----- INIT -----
    Lexer lexer;
    lexer_init(&lexer, char_stream);

    Parser parser;
    parser_init(&parser, &lexer);

    Sema sema;
    sema_init(&sema);

    TACGen tac;
    tac_init(&tac);

    //----- STEP 1 -----
    //scan and parse source program, O(n)
    ASTNode* root = parse_program(&parser);

    //----- STEP 2 -----
    //perform semantic analysis on AST, validate and annotate
    collect_functions(&sema, root); //1st pass, collect and store program function signatures
    sema_node(&sema, root); //2nd pass, walk AST tree and perform validation and annotation

    //----- STEP 3 -----
    //produce TAC intermediate representation of code from annotated AST, architecture-independent
    tac_node(&tac, root);    

    print_tac(&tac);
        
    free(char_stream);

    return 0;
}

