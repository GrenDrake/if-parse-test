#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

#define T_LIST    0
#define T_ATOM    1
#define T_STRING  2
#define T_INTEGER 3

#define T_OPEN    98
#define T_CLOSE   99

typedef struct TOKEN {
    int type;
    int number;
    char *text;

    struct TOKEN *prev;
    struct TOKEN *next;
} token_t;


typedef struct LIST {
    int type;
    int number;
    char *text;
    struct LIST *child;

    struct LIST *next;
} list_t;


char *str_dupl(const char *text) {
    size_t length = strlen(text);
    char *new_text = malloc(length + 1);
    strcpy(new_text, text);
    return new_text;
}

int valid_identifier(int ch) {
    if (isalnum(ch) || ch == '-' || ch == '_') {
        return 1;
    }
    return 0;
}

void token_add(token_t **tokens, token_t *token) {
    if (*tokens == NULL) {
        *tokens = token;
    } else {
        token->prev = *tokens;
        (*tokens)->next = token;
        *tokens = token;
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

list_t* parse_list(token_t **place) {
    token_t *cur = *place;

    if (cur->type != T_OPEN) {
        printf("Expected '('\n");
        return NULL;
    }
    cur = cur->next;

    list_t *end_ptr = NULL;
    list_t *list = calloc(sizeof(list_t), 1);
    list->type = T_LIST;
    while (cur && cur->type != T_CLOSE) {
        if (cur->type == T_OPEN) {
            list_t *sublist = parse_list(&cur);
            if (!list->child) {
                list->child = sublist;
                end_ptr = sublist;
            } else {
                end_ptr->next = sublist;
                end_ptr = sublist;
            }
        } else {
            if (cur->type == T_INTEGER) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_INTEGER;
                item->number = cur->number;
                if (!list->child) {
                    list->child = item;
                    end_ptr = item;
                } else {
                    end_ptr->next = item;
                    end_ptr = item;
                }
            } else if (cur->type == T_ATOM) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_ATOM;
                item->text = str_dupl(cur->text);
                if (!list->child) {
                    list->child = item;
                    end_ptr = item;
                } else {
                    end_ptr->next = item;
                    end_ptr = item;
                }
            } else if (cur->type == T_STRING) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_STRING;
                item->text = str_dupl(cur->text);
                if (!list->child) {
                    list->child = item;
                    end_ptr = item;
                } else {
                    end_ptr->next = item;
                    end_ptr = item;
                }
            }
            cur = cur->next;
        }

    }
    if (cur == NULL) {
        printf("Unexpected end of tokens\n");
        *place = cur;
        return NULL;
    }
    cur = cur->next;

    *place = cur;
    return list;
}

int parse_object(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"object")) {
        return 0;
    }

    object_t *obj = object_create(gd->root);
    list_t *prop = list->child->next;
    list_t *val  = prop->next;
    if (prop->type != T_ATOM || val->type != T_ATOM) {
        printf("Object name and parent must be atom.\n");
        return 0;
    }
    if (strcmp(prop->text, "-") != 0) {
        object_property_add_string(obj, PI_IDENT, str_dupl(prop->text));
    }
    if (strcmp(val->text, "-") != 0) {
        obj->parent_name = val->text;
        printf("guv %p %p %s\n", obj->parent_name, val->text, val->text);
    }
    while (prop) {
        if (val == NULL) {
            printf("Found property without value.\n");
            return 0;
        }
        prop = val->next;
        if (prop) {
            val = prop->next;
        } else {
            val = NULL;
        }
    }

    return 1;
}

void print_list(list_t *list) {
    list_t *pos = list->child;
    printf("{");
    while (pos) {
        switch(pos->type) {
            case T_LIST:
                printf(" [LIST: ");
                print_list(pos);
                printf("]");
                break;
            case T_STRING:
                printf(" [STR: %s]", pos->text);
                break;
            case T_ATOM:
                printf(" [ATM: %s]", pos->text);
                break;
            case T_INTEGER:
                printf(" [INT: %d]", pos->number);
                break;
            default:
                printf(" [%d: unhandled]", pos->type);
        }
        pos = pos->next;
    }
    printf(" }");
}


gamedata_t* parse_file(const char *filename) {
    //  read the data file
    printf("Loading game data...\n");
    FILE *fp = fopen(filename, "rt");
    if (!fp) {
        fprintf(stderr, "Could not open file '%s'\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *file = malloc(filesize+1);
    fread(file, filesize, 1, fp);
    file[filesize] = 0;
    fclose(fp);

    token_t *tokens = NULL;

    printf("Tokenizing game data...\n");
    // tokenize the source data
    size_t pos = 0;
    while (pos < filesize) {
        if (file[pos] == '(') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_OPEN;
            token_add(&tokens, t);
            ++pos;
        } else if (file[pos] == ')') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_CLOSE;
            token_add(&tokens, t);
            ++pos;
        } else if (isdigit(file[pos])) {
            char *token = &file[pos];
            while (isdigit(file[pos])) {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_INTEGER;
            t->number = strtol(token, 0, 0);
            token_add(&tokens, t);
        } else if (valid_identifier(file[pos])) {
            char *token = &file[pos];
            while (valid_identifier(file[pos])) {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_ATOM;
            t->text = str_dupl(token);
            token_add(&tokens, t);
        } else if (file[pos] == '"') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '"') {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_STRING;
            t->text = str_dupl(token);
            token_add(&tokens, t);
        } else {
            ++pos;
        }
    }
    free(file);

    // reverse the token list
    while (tokens->prev) {
        tokens = tokens->prev;
    }

//    token_t *cur = tokens;
//    while (cur) {
//        printf("%d: ", cur->type);
//        if (cur->type == T_STRING) printf("~%s~", cur->text);
//        if (cur->type == T_ATOM) printf("=%s=", cur->text);
//        if (cur->type == T_INTEGER) printf("%d", cur->number);
//        printf(" (%p - %p - %p)\n", cur->prev, cur, cur->next);
//        cur = cur->next;
//    }

    printf("Parsing game data...\n");
    list_t *lists = NULL, *last_list = NULL;
    token_t *list_pos = tokens;
    while (list_pos) {
        list_t *list = parse_list(&list_pos);
        if (!list) {
            return NULL;
        }
        if (lists) {
            last_list->next = list;
            last_list = list;
        } else {
            lists = list;
            last_list = list;
        }
    }

    printf("Processing game data...\n");
    gamedata_t *gd = gamedata_create();
    list_t *clist = lists;
    while (clist) {
        if (clist->type != T_LIST) {
            printf("Expected list at top level.\n");
            return NULL;
        }
        if (clist->child == NULL) {
            printf("Empty list found at tope level.\n");
            return NULL;
        }
        if (clist->child->type != T_ATOM) {
            printf("Top-level list must start with atom.\n");
            return NULL;
        }

        if (strcmp(clist->child->text, "object") == 0) {
            if (!parse_object(gd, clist)) {
                return NULL;
            }

            printf("%d   ", list_size(clist));
            print_list(clist);
            printf("\n");
        } else {
            printf("Unknown top level construct %s.\n", clist->child->text);
            return NULL;
        }

        clist = clist->next;
    }

    printf("Parenting game objects...\n");
    object_t *curo = gd->root->first_child;
    while (curo) {
        if (curo->parent_name) {
            object_t *parent = object_get_by_ident(gd, curo->parent_name);
            if (!parent) {
                printf("Unknown object name %s.\n", curo->parent_name);
                return NULL;
            }
            object_move(curo, parent);
            curo->parent_name = NULL;
        }
        curo = curo->sibling;
    }

    // free tokens and lists
    token_t *next;
    while (tokens) {
        next = tokens->next;
        free(tokens);
        tokens = next;
    }
    
    list_t *next_list;
    while (lists) {
        next_list = lists->next;
        free(lists);
        lists = next_list;
    }
    return gd;
}

