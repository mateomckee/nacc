#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

//helper functions, AI generated

void error(int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "error on line %d: ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void* nacc_malloc(unsigned int size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return ptr;
}

void* nacc_realloc(void* ptr, unsigned int size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return new_ptr;
}
