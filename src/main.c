#include <stdio.h>
#include <stdlib.h>
#include "sema.h"

// simple function to take file contents and return them as a string
char* read_file(const char* filepath) {
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

// quick tester main
// pass in the filepath of the source program
//  ../tests/count.c
// currently just runs lexer and spits out tokens
int main(int argc, char* argv[]) {

    if(argc < 2) {
        printf("Please provide the filepath of the source code\n");
        return 1;
    }

    const char* filepath = argv[1];
    char* char_stream = read_file(filepath);

    //lexer
    Lexer lexer;
    lexer_init(&lexer, char_stream);

    //parser
    Parser parser;
    parser_init(&parser, &lexer);

    //semantic analysis
    Sema sema;
    sema_init(&sema);

    ASTNode* root = parse_program(&parser);

    print_ast(root, 0);

    //first pass sema
    collect_functions(&sema, root);
    //second pass sema
    sema_node(&sema, root);

    //

    free(char_stream);

    return 0;
}

