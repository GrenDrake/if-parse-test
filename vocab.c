#include <string.h>

#include "parse.h"


int vocab_index(const char *word) {
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
