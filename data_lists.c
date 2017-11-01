#include <stdio.h>
#include <stdlib.h>

#include "dparse.h"

const char *symbol_types[] = {
    "object",
    "property",
    "constant",
};


void dump_tokens(FILE *dest, token_t *tokens) {
    token_t *cur = tokens;
    while (cur) {
        fprintf(dest, "%d: ", cur->type);
        if (cur->type == T_STRING)
            fprintf(dest, "~%s~", cur->text);
        if (cur->type == T_ATOM)
            fprintf(dest, "=%s=", cur->text);
        if (cur->type == T_INTEGER)
            fprintf(dest, "%d", cur->number);
        if (cur->type == T_VOCAB)
            fprintf(dest, "<%s>", cur->text);
        fprintf(dest, " (%p - %p - %p)\n", (void*)cur->prev, (void*)cur, (void*)cur->next);
        cur = cur->next;
    }
}

void dump_list(FILE *dest, list_t *list) {
    list_t *pos = list->child;
    fprintf(dest, "{");
    while (pos) {
        switch(pos->type) {
            case T_LIST:
                fprintf(dest, " ");
                dump_list(dest, pos);
                break;
            case T_STRING:
                fprintf(dest, " ~%s~", pos->text);
                break;
            case T_ATOM:
                fprintf(dest, " %s", pos->text);
                break;
            case T_INTEGER:
                fprintf(dest, " %d", pos->number);
                break;
            case T_VOCAB:
                fprintf(dest, " <%d>", pos->number);
                break;
            default:
                fprintf(dest, " [%d: unhandled]", pos->type);
        }
        pos = pos->next;
    }
    fprintf(dest, " }");
}


/* ****************************************************************************
 * Token manipulation
 * ****************************************************************************/
 void token_add(token_t **tokens, token_t *token) {
    if (*tokens == NULL) {
        *tokens = token;
    } else {
        token->prev = *tokens;
        (*tokens)->next = token;
        *tokens = token;
    }
}

void token_free(token_t *token) {
    if (token->type == T_STRING || token->type == T_ATOM) {
        free(token->text);
    }
    free(token);
}


/* ****************************************************************************
 * List manipulation
 * ****************************************************************************/
void list_add(list_t *list, list_t *item) {
    if (!list || !item) return;
    if (list->child) {
        list->last->next = item;
        list->last = item;
    } else {
        list->child = list->last = item;
    }
}

list_t *list_create() {
    list_t *list = calloc(sizeof(list_t), 1);
    list->type = T_LIST;
    return list;
}

void list_free(list_t *list) {
    if (list->type == T_LIST) {
        list_t *sublist = list->child;
        while (sublist) {
            list_t *next = sublist->next;
            list_free(sublist);
            sublist = next;
        }
    } else if (list->type == T_ATOM || list->type == T_STRING || list->type == T_VOCAB) {
        free(list->text);
    }
    free(list);
}

int list_size(list_t *list) {
    if (list->type != T_LIST || list->child == NULL) {
        return 0;
    }

    int count = 0;
    list_t *counter = list->child;
    while (counter) {
        ++count;
        counter = counter->next;
    }
    return count;
}
