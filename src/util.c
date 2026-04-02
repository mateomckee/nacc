#include <stdlib.h>
#include <stdio.h>

void error(int line, char* msg) {
    fprintf(stderr, "Error on line %d: %s\n", line, msg);
    exit(1);
}
