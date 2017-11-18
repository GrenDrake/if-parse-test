#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

const char *symbol_types[] = {
    "object",
    "property",
    "constant",
    "function",
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
    if (!list) {
        fprintf(dest, "(NULL)");
        return;
    }

    list_t *pos;
    switch(list->type) {
        case T_LIST:
            pos = list->child;
            fprintf(dest, "{");
            while (pos) {
                fprintf(dest, " ");
                dump_list(dest, pos);
                pos = pos->next;
            }
            fprintf(dest, " }");
            break;
        case T_STRING:
            fprintf(dest, "~%s~", list->text);
            break;
        case T_ATOM:
            fprintf(dest, "%s", list->text);
            break;
        case T_INTEGER:
            fprintf(dest, "%d", list->number);
            break;
        case T_VOCAB:
            fprintf(dest, "<%d>", list->number);
            break;
        case T_OBJECT_REF:
            fprintf(dest, "object@%p", list->ptr);
            break;
        case T_FUNCTION_REF:
            fprintf(dest, "function@%p", list->ptr);
            break;
        default:
            fprintf(dest, "[%d: unhandled]", list->type);
    }

}


/* ****************************************************************************
 * Token manipulation
 * ****************************************************************************/

void token_free(token_t *token) {
    if (token->type == T_STRING || token->type == T_ATOM) {
        free(token->text);
    }
    free(token);
}

void token_freelist(token_t *tokens) {
    token_t *token = tokens;
    while (token) {
        token_t *next = token->next;
        token_free(token);
        token = next;
    }
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

list_t *list_create_false() {
    list_t *list = calloc(sizeof(list_t), 1);
    list->type = T_INTEGER;
    list->number = 0;
    return list;
}

list_t *list_create_true() {
    list_t *list = calloc(sizeof(list_t), 1);
    list->type = T_INTEGER;
    list->number = 1;
    return list;
}

list_t *list_duplicate(list_t *old_list) {
    if (!old_list) return NULL;

    list_t *iter = NULL;
    list_t *new_list = list_create();
    new_list->type = old_list->type;
    switch(old_list->type) {
        case T_LIST:
            iter = old_list->child;
            while (iter) {
                list_add(new_list, list_duplicate(iter));
                iter = iter->next;
            }
            break;
        case T_OBJECT_REF:
        case T_FUNCTION_REF:
            new_list->ptr = old_list->ptr;
            break;
        case T_STRING:
        case T_ATOM:
            new_list->text = str_dupl(old_list->text);
            break;
        case T_VOCAB:
        case T_INTEGER:
            new_list->number = old_list->number;
            break;
        default:
            debug_out("list_duplicate: unknown list type %d.\n", old_list->type);
            list_free(new_list);
            return NULL;
    }
    return new_list;
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

void list_freelist(list_t *lists) {
    list_t *next_list;
    while (lists) {
        next_list = lists->next;
        list_free(lists);
        lists = next_list;
    }
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

int list_is_true(list_t *list) {
    if (!list) {
        return FALSE;
    }

    switch(list->type) {
        case T_LIST:
            return list->child != NULL;
        case T_INTEGER:
            return list->number != 0;
        case T_STRING:
        case T_ATOM:
        case T_OBJECT_REF:
        case T_FUNCTION_REF:
            return TRUE;
        default:
            return FALSE;
    }
}
