#include <stdlib.h>
#include <stdio.h>

//helper functions, AI generated
void error(int line, char* msg) {
    fprintf(stderr, "Error on line %d: %s\n", line, msg);
    exit(1);
}

void* nacc_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return ptr;
}
