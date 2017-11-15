#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

void debug_out(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

void text_out(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stdout, msg, args);
    va_end(args);
}

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
