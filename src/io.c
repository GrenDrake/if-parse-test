#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

char* read_line() {
    char *buffer = calloc(MAX_INPUT_LENGTH, 1);
    fgets(buffer, MAX_INPUT_LENGTH-1, stdin);
    return buffer;
}

void style_bold() {
    printf("\x1b[1m");
}

void style_normal() {
    printf("\x1b[0m");
}

void style_reverse() {
    printf("\x1b[7m");
}
