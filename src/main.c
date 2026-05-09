#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen_aarch64.h"

//simple function to take file contents and return them as a string
char* get_file_as_string(char* filepath) {
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

char* get_output_filename(char* input_filepath) {
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

    return output_filename;
}


void print_file(char* filename) {
    FILE* out = fopen(filename, "r");

    char ch;
    while ((ch = fgetc(out)) != EOF) {
        printf("%c", ch);
    }

    fclose(out);
}


// pass in the filepath of the source program
//  ../tests/count.c
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Please provide the filepath of the source code\n");
        return 1;
    }

    char* input_filepath = argv[1];

    char* char_stream = get_file_as_string(input_filepath);
    char* output_filename = get_output_filename(input_filepath);

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
    codegen_init(&cg, &tac);

    //step 1
    //scan and parse source program, O(n)
    ASTNode* root = parse_program(&parser);

    //print_ast(root, 0);

    //step 2
    //perform semantic analysis on AST, validate and annotate
    collect_functions(&sema, root); //1st pass, collect and store program function signatures
    sema_node(&sema, root); //2nd pass, walk AST tree and perform validation and annotation

    //step 3
    //produce TAC intermediate representation of code from annotated AST, architecture-independent
    tac_node(&tac, root);    

    //print_tac(&tac);

    //step 4
    //walk linear TAC code and print AArch64 assembly into output file. codegen_run encompasses file lifetime
    codegen_run(&cg, output_filename);    

    //print_file(output_filename);

    free(char_stream);

    return 0;
}

