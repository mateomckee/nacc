#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen_aarch64.h"

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

FILE* get_output_file(const char* input_filepath) {
    //basename
    char* basename;
    //string split token
    char* token;

    token = strtok(input_filepath, "/");
    while(token != NULL) {
        basename = token;
        token = strtok(NULL, "/");
    }

    //grab first split, filename before extension
    char* name = strtok(basename, ".");

    char* output_filename = strcat(name, ".s");

    FILE* out = fopen(output_filename, "w");
    if(out == NULL) {
        fprintf(stderr, "could not open file %s\n", name);
        exit(1);
    }

    return out;
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

    //open output file for codegen
    FILE* out = get_output_file(filepath);

    //init
    Lexer lexer;
    lexer_init(&lexer, char_stream);

    Parser parser;
    parser_init(&parser, &lexer);

    Sema sema;
    sema_init(&sema);

    TACGen tac;
    tac_init(&tac);

    CodeGen cg;
    codegen_init(&cg, &tac, out);

    //step 1
    //scan and parse source program, O(n)
    ASTNode* root = parse_program(&parser);

    //step 2
    //perform semantic analysis on AST, validate and annotate
    collect_functions(&sema, root); //1st pass, collect and store program function signatures
    sema_node(&sema, root); //2nd pass, walk AST tree and perform validation and annotation

    //step 3
    //produce TAC intermediate representation of code from annotated AST, architecture-independent
    tac_node(&tac, root);    

    print_tac(&tac);

    //step 4
    //walk linear TAC code and print AArch64 assembly into output file
    codegen_run(&cg);    

    free(char_stream);
    fclose(out);

    return 0;
}

