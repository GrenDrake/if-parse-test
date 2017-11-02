#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

object_t* scope_ceiling(gamedata_t *gd, object_t *obj);
int word_in_property(object_t *obj, int pid, int word);
void add_to_scope(gamedata_t *gd, object_t *obj);
void scope_within(gamedata_t *gd, object_t *ceiling);

void object_name_print(gamedata_t *gd, object_t *obj);
void object_property_print(object_t *obj, int prop_num);
void print_location(gamedata_t *gd, object_t *location);
int dispatch_action(gamedata_t *gd, input_t *input);
int game_init(gamedata_t *gd);


/* ************************************************************************ *
 * Interacting with the world model
 * ************************************************************************ */

 object_t* scope_ceiling(gamedata_t *gd, object_t *obj) {
    if (!obj) return NULL;
    while (obj->parent != gd->root) {
        obj = obj->parent;
    }
    return obj;
}

int word_in_property(object_t *obj, int pid, int word) {
    property_t *p = object_property_get(obj, pid);
    if (!p || p->value.type != PT_ARRAY) {
        return 0;
    }
    for (int i = 0; i < p->value.array_size; ++i) {
        value_t *v = &((value_t*)p->value.d.ptr)[i];
        if (word == v->d.num) {
            return 1;
        }
    }
    return 0;
}

void add_to_scope(gamedata_t *gd, object_t *obj) {
    gd->search[gd->search_count] = obj;
    ++gd->search_count;
}

void scope_within(gamedata_t *gd, object_t *ceiling) {
    object_t *obj, *obj_list[16];
    int queue;

    queue = 1;
    obj_list[0] = ceiling->first_child;
    obj = ceiling->first_child;
    while (queue > 0) {
        --queue;
        object_t *here = obj_list[queue];
        add_to_scope(gd, here);

        if (here->sibling) {
            obj_list[queue] = here->sibling;
            ++queue;
        }
        if (here->first_child) {
            obj_list[queue] = here->first_child;
            ++queue;
        }
    }
}

/* ************************************************************************ *
 * Tokenizing player input
 * ************************************************************************ */

/**
Takes text input by the player and turns it into a sequence of words and
vocab word numbers.

Returns false if the input text was empty or contained only whitespace;
returns true otherwise.
*/
int tokenize(input_t *input) {
    int in_word = 0, count = 0;
    memset(input->words, 0, sizeof(cmd_token_t) * MAX_INPUT_WORDS);

    if (!input) {
        input->words[0].word_no = -1;
        return 0;
    }

    for (int i = 0; input->input[i]; ++i) {
        unsigned char here = input->input[i];

        if (isspace(here) || here == 0) {
            if (in_word) {
                in_word = 0;
                input->input[i] = 0;
                input->words[count].word_no = vocab_index(input->words[count].word);
                ++count;
            }
            if (here != 0) {
                continue;
            } else {
                break;
            }
        }

        if (!in_word) {
            input->words[count].word = &input->input[i];
            in_word = 1;
        }
    }
    if (count == 0) {
        printf("Pardon?\n");
        return 0;
    } else {
        input->word_count = count;
        input->words[count].word_no = -1;
    }

    return 1;
}

/* ************************************************************************ *
 * Parsing tokenized input
 * ************************************************************************ */

 /**
 Try and match part of the player's input to a noun in the currently
 defined scope.

 Returns the object found if a single match is made, returns NULL if
 no match is made, and returns OBJ_AMBIG if multiple matches exist, but
 a decision between them could not be made.
 */
object_t* match_noun(gamedata_t *gd, input_t *input) {
    object_t *match = NULL;
    int match_strength = 0;
    int prop_vocab = property_number(gd, "vocab");

    printf("finding noun...\n");
    for (int i = 0; i < gd->search_count; ++i) {
        printf("   OBJECT ");
        object_property_print(gd->search[i], property_number(gd, "name"));

        int words = 0;
        int cur_word = input->cur_word;
        while (word_in_property(gd->search[i], prop_vocab,
                                input->words[cur_word].word_no)) {
            ++words;
            ++cur_word;
        }
        printf(" [%d]", words);
        if (words > match_strength) {
            printf("   (match)\n");
            match = gd->search[i];
            match_strength = words;
        } else if (words == match_strength && words > 0) {
            printf("   (too many!)\n");
            match = OBJ_AMBIG;
        } else {
            printf("\n");
        }
    }

    input->cur_word += match_strength;
    return match;
}

/**
Try to parse the player's input as a specific action.

Returns the action's action number if successful. On failure, returns an
error code less than 0.
*/
int try_parse_action(gamedata_t *gd, input_t *input, action_t *action) {
    int token_no = 0;
    input->cur_word = 0;
    object_t *obj;

    while (1) {
        if (action->grammar[token_no].type == GT_END) {
            if (input->cur_word == input->word_count) {
                return action->action_code;
            } else {
                return PARSE_NONMATCH;
            }
        } else if (input->cur_word >= input->word_count) {
            return PARSE_NONMATCH;
        }

        gd->search_count = 0;
        switch(action->grammar[token_no].type) {
            case GT_END:
                printf("PARSE ERROR: Encountered GT_END in grammar; this should have already been handled.\n");
                break;
            case GT_NOUN:
                obj = scope_ceiling(gd, gd->player);
                add_to_scope(gd, obj);
                scope_within(gd, obj);
                obj = match_noun(gd, input);
                if (!obj) {
                    return token_no ? PARSE_BADNOUN : PARSE_NONMATCH;
                } else if (obj == OBJ_AMBIG) {
                    return PARSE_AMBIG;
                } else {
                    input->nouns[input->noun_count] = obj;
                    ++input->noun_count;
                }
                ++token_no;
                break;
            case GT_SCOPE:
                scope_within(gd, (object_t*)action->grammar[token_no].ptr);
                obj = match_noun(gd, input);
                if (!obj) {
                    return token_no ? PARSE_BADNOUN : PARSE_NONMATCH;
                } else if (obj == OBJ_AMBIG) {
                    return PARSE_AMBIG;
                } else {
                    input->nouns[input->noun_count] = obj;
                    ++input->noun_count;
                }
                ++token_no;
                break;
            case GT_ANY:
                ++token_no;
                ++input->cur_word;
                break;
            case GT_WORD:
                if (input->words[input->cur_word].word_no == action->grammar[token_no].value) {
                    while (action->grammar[token_no].flags & GF_ALT) {
                        ++token_no;
                    }
                    ++token_no;
                    ++input->cur_word;
                } else {
                    if (action->grammar[token_no].flags & GF_ALT) {
                        ++token_no;
                    } else {
                        return PARSE_NONMATCH;
                    }
                }
                break;
            default:
                return PARSE_BADTOKEN;
        }
    }
}

/**
Try the parse the player's tokenize command as an action. If a match is
found, the relevent fields in the input data will be filled out.

Returns false if not matching action line could be found; returns true
otherwise.
*/
int parse(gamedata_t *gd, input_t *input) {
    action_t *action_iter = gd->actions;
    input->noun_count = 0;

    if (input->words[0].word_no == -1) {
        printf("Unknown word '%s'.\n", input->words[0].word);
        input->action = PARSE_BADTOKEN;
        return 0;
    }

    int best_result = PARSE_BADTOKEN;
    while (action_iter && best_result < 0) {
        int result = try_parse_action(gd, input, action_iter);
        if (best_result < result) {
            best_result = result;
        }
        if (result >= 0) {
            break;
        }
        action_iter = action_iter->next;
    }

    switch(best_result) {
        case PARSE_AMBIG:
            printf("Multiple items matched.\n");
            break;
        case PARSE_BADNOUN:
            printf("Not visible.\n");
            break;
        case PARSE_NONMATCH:
            printf("Unrecognized verb '%s'.\n", input->words[0].word);
            break;
        case PARSE_BADTOKEN:
            printf("Parser error.\n");
            break;
        default:
            input->action = best_result;
    }

    return input->action >= 0;
}

/* ************************************************************************ *
 * Main game loop
 * ************************************************************************ */

int game_init(gamedata_t *gd) {
    gd->gameinfo = object_get_by_ident(gd, "gameinfo");
    if (!gd->gameinfo) {
        printf("FATAL: no gameinfo object\n");
        free_data(gd);
        return 0;
    }

    property_t *player_prop = object_property_get(gd->gameinfo, property_number(gd, "player"));
    if (!player_prop || !player_prop->value.d.ptr) {
        printf("FATAL: gameinfo does not define valid initial player object\n");
        free_data(gd);
        return 0;
    }
    gd->player = player_prop->value.d.ptr;

    printf("if-parse-test VERSION %d.%d.%d\n\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
    printf("--------------------------------------------------------------------------------\n\n");

    property_t *intro_prop = object_property_get(gd->gameinfo, property_number(gd, "intro"));
    if (intro_prop && intro_prop->value.d.ptr) {
        printf("%s\n", intro_prop->value.d.ptr);
    }

    return 1;
}



int main() {
    gamedata_t *gd = load_data();
    if (!gd) return 1;
    if (!game_init(gd)) return 1;

    print_location(gd, gd->player->parent);
    while (!gd->quit_game) {
        printf("\n> ");
        input_t input = {0};
        input.input = read_line();

        if (!tokenize(&input)) {
            printf("Pardon?\n");
            free(input.input);
            continue;
        }

        if (!parse(gd, &input)) {
            free(input.input);
            continue;
        }

        dispatch_action(gd, &input);
        free(input.input);
    }

    printf("Goodbye!\n\n");
    free_data(gd);
    return 0;
}
