#ifndef DPARSE_H
#define DPARSE_H

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

extern const char *symbol_types[];


void dump_list(FILE *dest, list_t *list);
void dump_tokens(FILE *dest, token_t *tokens);

void token_add(token_t **tokens, token_t *token);
void token_free(token_t *token);

void list_add(list_t *list, list_t *item);
list_t *list_create();
void list_free(list_t *list);
int list_size(list_t *list);

#endif  // DPARSE_H
