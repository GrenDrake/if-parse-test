#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

void debug_out(const char *msg, ...) {
    static FILE *logfile = NULL;

    if (logfile == NULL) {
        logfile = fopen("debug.log", "wt");
        setvbuf(logfile, NULL, _IONBF, 0);
    }


    va_list args;
    va_start(args, msg);
    vfprintf(logfile, msg, args);
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

char* read_file(const char *filename) {
    FILE *fp = fopen(filename, "rt");
    if (!fp) {
        debug_out("read_file: Could not open file '%s'\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *file = malloc(filesize+1);
    fread(file, filesize, 1, fp);
    file[filesize] = 0;
    fclose(fp);
    return file;
}

void style_bold() {
    text_out("\x1b[1m");
}

void style_normal() {
    text_out("\x1b[0m");
}

void style_reverse() {
    text_out("\x1b[7m");
}
