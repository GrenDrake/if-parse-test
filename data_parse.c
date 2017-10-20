#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

#define T_LIST    0
#define T_ATOM    1
#define T_STRING  2
#define T_INTEGER 3
#define T_VOCAB   4

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
    struct LIST *last;

    struct LIST *next;
} list_t;

// dumping data
static void dump_list(FILE *dest, list_t *list);
static void dump_lists(FILE *fp, list_t *lists);
void dump_symbol_table(FILE *fp, gamedata_t *gd);
static void dump_tokens(FILE *dest, token_t *tokens);

// list manipulation
void list_add(list_t *list, list_t *item);
list_t *list_create();
void list_free(list_t *list);
int list_size(list_t *list);

// token manipulation
static void token_add(token_t **tokens, token_t *token);
static void token_free(token_t *token);

// tokenizing
static int valid_identifier(int ch);
static token_t *tokenize(char *file);

// parsing
static int parse_action(gamedata_t *gd, list_t *list);
static list_t* parse_list(token_t **place);
static int parse_object(gamedata_t *gd, list_t *list);


/* ****************************************************************************
 * Dumping data to a stream (for debugging)
 * ****************************************************************************/
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

void dump_lists(FILE *fp, list_t *lists) {
    while(lists) {
        dump_list(fp, lists);
        fprintf(fp, "\n");
        lists = lists->next;
    }
}

void dump_symbol_table(FILE *fp, gamedata_t *gd) {
    fprintf(fp, "==================================================\n");
    fprintf(fp, "Symbol Name                       Type  Value\n");
    fprintf(fp, "--------------------------------------------------\n");
    for (int i = 0; i < SYMBOL_TABLE_BUCKETS; ++i) {
        symbol_t *symbol = gd->symbols->buckets[i];
        while (symbol) {
            fprintf(fp, "%-32s  %d     %p\n", symbol->name, symbol->type, symbol->d.ptr);
            symbol = symbol->next;
        }
    }
    fprintf(fp, "==================================================\n");
}

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
        fprintf(dest, " (%p - %p - %p)\n", cur->prev, cur, cur->next);
        cur = cur->next;
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
 * Tokenizing
 * ****************************************************************************/
int valid_identifier(int ch) {
    if (isalnum(ch) || ch == '-' || ch == '_') {
        return 1;
    }
    return 0;
}

token_t *tokenize(char *file) {
    if (!file) return NULL;
    
    token_t *tokens = NULL;
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
        } else if (file[pos] == '<') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '>') {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_VOCAB;
            vocab_raw_add(token);
            t->text = str_dupl(token);
            token_add(&tokens, t);
        } else {
            printf("Unexpected token '%c' (%d).\n", file[pos], file[pos]);
            ++pos;
        }
    }
    return tokens;
}


/* ****************************************************************************
 * Parse functions
 * ****************************************************************************/
int parse_action(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"action") || list->child->next == NULL) {
        return 0;
    }

    list_t *cur = list->child->next;
    if (cur->type != T_INTEGER) {
        printf("Action number must be integer.\n");
        return 0;
    }
    int code = cur->number;
    cur = cur->next;
    if (!cur) {
        printf("Action has no grammar.\n");
        return 0;
    }
    
    action_t *act = calloc(sizeof(action_t), 1);
    act->action_code = code;
    int pos = 0;

    list_t *sub;
    while (cur) {
        switch(cur->type) {
            case T_VOCAB:
                act->grammar[pos].type = GT_WORD;
                act->grammar[pos].value = vocab_index(cur->text);
                ++pos;
                break;
            case T_ATOM:
                if (strcmp(cur->text, "noun") == 0) {
                    act->grammar[pos].type = GT_NOUN;
                    ++pos;
                } else if (strcmp(cur->text, "any") == 0) {
                    act->grammar[pos].type = GT_ANY;
                    ++pos;
                } else {
                    printf("Unrecognized grammar token '%s'.\n", cur->text);
                    return 0;
                }
                break;
            case T_LIST:
                sub = cur->child;
                while (sub) {
                    switch (sub->type) {
                        case T_VOCAB:
                            act->grammar[pos].type = GT_WORD;
                            act->grammar[pos].value = vocab_index(sub->text);
                            if (sub->next) {
                                act->grammar[pos].flags |= GF_ALT;
                            }
                            ++pos;
                            break;
                        default:
                            printf("Only vocab values permitted in action sub-statement. (%d)\n", sub->type);
                            return 0;
                    }
                    sub = sub->next;
                }
                break;
            case T_STRING:
            case T_INTEGER:
                printf("Integer and string values not permitted in action statement.\n");
                return 0;
            default:
                printf("Bad token type %d in action definition.\n", cur->type);
        }
        cur = cur->next;
    }
    action_add(gd, act);
    
    return 1;
}

list_t* parse_list(token_t **place) {
    token_t *cur = *place;

    if (cur->type != T_OPEN) {
        printf("Expected '('\n");
        return NULL;
    }
    cur = cur->next;

    list_t *list = list_create();
    while (cur && cur->type != T_CLOSE) {
        if (cur->type == T_OPEN) {
            list_t *sublist = parse_list(&cur);
            list_add(list, sublist);
        } else {
            if (cur->type == T_INTEGER) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_INTEGER;
                item->number = cur->number;
                list_add(list, item);
            } else if (cur->type == T_ATOM) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_ATOM;
                item->text = str_dupl(cur->text);
                list_add(list, item);
            } else if (cur->type == T_STRING) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_STRING;
                item->text = str_dupl(cur->text);
                list_add(list, item);
            } else if (cur->type == T_VOCAB) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_VOCAB;
                item->text = str_dupl(cur->text);
                list_add(list, item);
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
        symbol_add_ptr(gd, prop->text, SYM_OBJECT, obj);
    }
    if (strcmp(val->text, "-") != 0) {
        obj->parent_name = val->text;
    }
    prop = val->next;
    if (prop) val = prop->next;
    while (prop) {
        if (val == NULL) {
            printf("Found property without value.\n");
            return 0;
        }
        if (prop->type != T_ATOM) {
            printf("Property name must be atom.\n");
            return 0;
        }
        
        int p_num = property_number(gd, prop->text);
        if (val->type == T_STRING) {
            object_property_add_string(obj, p_num, str_dupl(val->text));
        } else if (val->type == T_ATOM) {
            object_property_add_string(obj, p_num, str_dupl(val->text));
            property_t *p = object_property_get(obj, p_num);
            p->value.type = PT_TMPNAME;
        } else if (val->type == T_INTEGER) {
            object_property_add_integer(obj, p_num, val->number);
        } else if (val->type == T_LIST) {
            object_property_add_array(obj, p_num, list_size(val));
            property_t *p = object_property_get(obj, p_num);
            value_t *arr = p->value.d.ptr;
            list_t *cur = val->child;
            int counter = 0;
            while (cur) {
                switch(cur->type) {
                    case T_INTEGER:
                        arr[counter].type = PT_INTEGER;
                        arr[counter].d.num = cur->number;
                        break;
                    case T_STRING:
                        arr[counter].type = PT_STRING;
                        arr[counter].d.ptr = str_dupl(cur->text);
                        break;
                    case T_ATOM:
                        arr[counter].type = PT_TMPNAME;
                        arr[counter].d.ptr = str_dupl(cur->text);
                        break;
                    case T_LIST: 
                        printf("Nested lists are not permitted in object properties.\n");
                        break;
                    default:
                        printf("WARNING: unhandled array value type %d.\n", cur->type);
                }
                ++counter;
                cur = cur->next;
            }
        } else {
            printf("WARNING: unhandled property type %d.\n", val->type);
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

gamedata_t* parse_file(const char *filename) {
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


    printf("Tokenizing game data...\n");
    token_t *tokens = tokenize(file);
    free(file);


    // reverse the token list
    while (tokens->prev) {
        tokens = tokens->prev;
    }
    vocab_build();

    printf("Building lists...\n");
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

    printf("Parsing lists...\n");
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
        } else if (strcmp(clist->child->text, "action") == 0) {
            if (!parse_action(gd, clist)) {
                return NULL;
            }
        } else {
            printf("Unknown top level construct %s.\n", clist->child->text);
            return NULL;
        }

        clist = clist->next;
    }

    printf("Dumpng symbol table...\n");
    dump_symbol_table(stdout, gd);

    printf("Setting object references...\n");
    object_t *curo = gd->root->first_child;
    while (curo) {
        object_t *next = curo->sibling;
        if (curo->parent_name) {
            object_t *parent = object_get_by_ident(gd, curo->parent_name);
            if (!parent) {
                printf("Unknown object name %s.\n", curo->parent_name);
                return NULL;
            }
            object_move(curo, parent);
            curo->parent_name = NULL;
        }
        
        property_t *p = curo->properties;
        while (p) {
            property_t *next = p->next;
            if (p->value.type == PT_TMPNAME) {
                object_t *obj = object_get_by_ident(gd, p->value.d.ptr);
                if (!obj) {
                    printf("Undefined reference to %s.\n", p->value.d.ptr);
                    return NULL;
                }
                object_property_add_object(curo, p->id, obj);
            }
            p = next;
        }
        
        curo = next;
    }

    printf("Data loaded. Freeing temporary memory...\n\n");
    token_t *next;
    while (tokens) {
        next = tokens->next;
        token_free(tokens);
        tokens = next;
    }
    list_t *next_list;
    while (lists) {
        next_list = lists->next;
        list_free(lists);
        lists = next_list;
    }

    vocab_dump();
    return gd;
}

