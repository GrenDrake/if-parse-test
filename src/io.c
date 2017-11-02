#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

void style_bold() {
    printf("\x1b[1m");
}

void style_normal() {
    printf("\x1b[0m");
}

void style_reverse() {
    printf("\x1b[7m");
}