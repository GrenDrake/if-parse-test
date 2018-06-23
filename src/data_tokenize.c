#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"


static int escape_string(char *text);
static int valid_identifier(int ch);
static void token_add(token_t **tokens, token_t **last_ptr, token_t *token);

token_t *tokenize_source(char *file, int allow_new_vocab);



int escape_string(char *text) {
    int found_error = FALSE;
    if (!text || text[0] == 0) {
        return FALSE;
    }

    for (int i = 0; i < strlen(text); ++i) {
        if (text[i] == '\\') {
            int escape_char = text[i+1];
            switch(escape_char) {
                case 0:
                    debug_out("escape_string: incomplete escape at end of string\n");
                    found_error = TRUE;
                    break;
                case 'n':
                    memmove(&text[i], &text[i+1], strlen(text) - i);
                    text[i] = '\n';
                    break;
                default:
                    memmove(&text[i], &text[i+1], strlen(text) - i);
                    debug_out("escape_string: unrecognized escape \\%c\n", escape_char);
                    found_error = TRUE;
                }
        }
    }

    return found_error;
}

int valid_identifier(int ch) {
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '#') {
        return 1;
    }
    return 0;
}

void token_add(token_t **tokens, token_t **last_ptr, token_t *token) {
    if (*last_ptr == NULL) {
        *tokens = *last_ptr = token;
    }
    else {
        (*last_ptr)->next = token;
        *last_ptr = token;
    }
}

token_t *tokenize_source(char *file, int allow_new_vocab) {
    if (!file) return NULL;

    token_t *tokens = NULL, *last_ptr = NULL;
    size_t pos = 0, filesize = strlen(file);
    while (pos < filesize) {
        if (file[pos] == '/' && pos+1 < filesize && file[pos+1] == '/') {
            while (pos < filesize && file[pos] != '\n') {
                ++pos;
            }
        } else if (isspace(file[pos])) {
            ++pos;
        } else if (file[pos] == '(') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_OPEN;
            token_add(&tokens, &last_ptr, t);
            ++pos;
        } else if (file[pos] == ')') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_CLOSE;
            token_add(&tokens, &last_ptr, t);
            ++pos;
        } else if (isdigit(file[pos])) {
            int start = pos;
            char *token = &file[pos];
            while (isdigit(file[pos])) {
                ++pos;
            }
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_INTEGER;
            char *tmp = str_dupl_left(token, pos - start);
            t->number = strtol(tmp, 0, 0);
            free(tmp);
            token_add(&tokens, &last_ptr, t);
        } else if (valid_identifier(file[pos])) {
            int start = pos;
            char *token = &file[pos];
            while (valid_identifier(file[pos])) {
                ++pos;
            }
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_ATOM;
            t->text = str_dupl_left(token, pos - start);
            token_add(&tokens, &last_ptr, t);
        } else if (file[pos] == '"') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '"') {
                ++pos;
            }
            file[pos++] = 0;
            escape_string(token);
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_STRING;
            t->text = str_dupl(token);
            token_add(&tokens, &last_ptr, t);
        } else if (file[pos] == '<') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '>') {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            if (allow_new_vocab) {
                vocab_raw_add(token);
                t->text = str_dupl(token);
                t->type = T_VOCAB;
            } else {
                t->number = vocab_index(token);
                t->type = T_INTEGER;
            }
            token_add(&tokens, &last_ptr, t);
        } else {
            text_out("Unexpected token '%c' (%d).\n", file[pos], file[pos]);
            ++pos;
        }
    }
    return tokens;
}
