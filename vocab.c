#include <stdlib.h>
#include <string.h>

#include "parse.h"

typedef struct VOCAB {
    char *word;
    struct VOCAB *prev;
    struct VOCAB *next;
} vocab_t;

static char **vocab;
static unsigned vocab_size = 0;
static vocab_t *vocab_list = NULL;

void vocab_dump() {
    if (!vocab) {
        printf("No vocab!\n");
        return;
    }
    
    for (int i = 0; i < vocab_size; ++i) {
        printf("%2d -%s-\n", i, vocab[i]);
    }
    printf("vocab size %d\n", vocab_size);
}

void vocab_raw_add(const char *the_word) {
    vocab_t *word = calloc(sizeof(vocab_t), 1);
    word->word = str_dupl(the_word);
    ++vocab_size;
    
    if (!vocab_list) {
        vocab_list = word;
        return;
    }
    
    if (strcmp(the_word, vocab_list->word) < 0) {
        word->next = vocab_list;
        vocab_list = word;
        return;
    }
    if (strcmp(the_word, vocab_list->word) == 0) {
        free(word->word);
        free(word);
        --vocab_size;
        return;
    }    
    
    vocab_t *cur = vocab_list;
    while (cur->next) {
        if (strcmp(the_word, cur->next->word) < 0) {
            word->next = cur->next;
            cur->next = word;
            return;
        }
        if (strcmp(the_word, cur->next->word) == 0) {
            free(word->word);
            free(word);
            --vocab_size;
            return;
        }    
        cur = cur->next;
    }
    cur->next = word;
}

void vocab_build() {
    vocab = calloc(sizeof(char*), vocab_size+1);
    vocab_t *cur = vocab_list;
    for (unsigned i = 0; i < vocab_size; ++i) {
        vocab[i] = cur->word;
        cur = cur->next;
    }
    vocab_raw_free(0);
}

void vocab_raw_free(int free_words) {
    vocab_t *vocab = vocab_list;
    while (vocab) {
        vocab_t *next = vocab->next;
        if (free_words) {
            free(vocab->word);
        }
        free(vocab);
        vocab = next;
    }
}

int vocab_index(const char *word) {
    if (!vocab) return -1;
    for (int i = 0; vocab[i] != 0; ++i) {
        if (strcmp(vocab[i], word) == 0) {
            return i;
        }
    }
    return -1;
}


int action_add(gamedata_t *gd, action_t *action) {
    if (!gd->actions) {
        gd->actions = action;
    } else {
        action->next = gd->actions;
        gd->actions = action;
    }
    return 1;
}
