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
    object_t *obj_list[16];
    int queue;

    queue = 1;
    obj_list[0] = ceiling->first_child;
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
        char punct_char = 0;

        if (ispunct(here) || isspace(here) || here == 0) {
            if (ispunct(here)) {
                punct_char = here;
            }
            if (in_word) {
                in_word = 0;
                input->input[i] = 0;
                input->words[count].word_no = vocab_index(input->words[count].word);
                ++count;
            }
            if (punct_char) {
                char buf[2] = { punct_char };
                input->words[count].word_no = vocab_index(buf);
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

void free_noun_list(noun_t *noun) {
    if (noun == NULL) return;

    free(noun->ambig);
    free(noun->also);
    free(noun);
}

 /**
 Try and match part of the player's input to a noun in the currently
 defined scope.

 Returns a noun object found if a single match is made, returns NULL if
 no match is made, and returns a linked list of noun objects if multiple
 matches exist, but a decision between them could not be made.
 */
noun_t* match_noun(gamedata_t *gd, input_t *input) {
    int match_strength = 0;
    noun_t *match = NULL;
    int prop_vocab = property_number(gd, "#vocab");

    text_out("finding noun...\n");
    for (int i = 0; i < gd->search_count; ++i) {
        text_out("   OBJECT ");
        object_property_print(gd->search[i], property_number(gd, "#name"));

        int words = 0;
        int cur_word = input->cur_word;
        while (word_in_property(gd->search[i], prop_vocab,
                                input->words[cur_word].word_no)) {
            ++words;
            ++cur_word;
        }
        text_out(" [%d]", words);
        noun_t *new_match = NULL;
        if (words > match_strength) {
            text_out("   (match)\n");
            new_match = calloc(sizeof(noun_t), 1);
            new_match->object = gd->search[i];
            match_strength = words;
            if (match != NULL) {
                free_noun_list(match);
            }
            match = new_match;
        } else if (words == match_strength && words > 0) {
            text_out("   (ambig)\n");
            new_match = calloc(sizeof(noun_t), 1);
            new_match->object = gd->search[i];
            match_strength = words;
            new_match->ambig = match->ambig;
            match->ambig = new_match;
        } else {
            text_out("\n");
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
    int then_word = vocab_index("then");
    int period_word = vocab_index(".");
    int token_no = 0;
    object_t *obj;
    noun_t *noun;

    while (1) {
        if (action->grammar[token_no].type == GT_END) {
            if (input->cur_word == input->word_count) {
                return action->action_code;
            } else if (input->words[input->cur_word].word_no == then_word
                        || input->words[input->cur_word].word_no == period_word) {
                ++input->cur_word;
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
            text_out("PARSE ERROR: Encountered GT_END in grammar; this should have already been handled.\n");
                break;
            case GT_SCOPE:
            case GT_NOUN:
                if (action->grammar[token_no].type == GT_SCOPE) {
                    scope_within(gd, (object_t*)action->grammar[token_no].ptr);
                } else {
                    obj = scope_ceiling(gd, gd->player);
                    add_to_scope(gd, obj);
                    scope_within(gd, obj);
                }
                noun = match_noun(gd, input);
                if (!noun) {
                    return token_no ? PARSE_BADNOUN : PARSE_NONMATCH;
                } else {
                    input->nouns[input->noun_count] = noun;
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

    if (input->words[input->cur_word].word_no == -1) {
        text_out("Unknown word '%s'.\n", input->words[input->cur_word].word);
        input->action = PARSE_BADTOKEN;
        return 0;
    }

    int best_result_end_word = 0;
    int best_result = PARSE_BADTOKEN;
    while (action_iter && best_result < 0) {
        input->cur_word = input->next_cmd;
        int result = try_parse_action(gd, input, action_iter);
        if (best_result < result) {
            best_result = result;
            best_result_end_word = input->cur_word;
        }
        if (result >= 0) {
            break;
        }
        action_iter = action_iter->next;
    }
    input->cur_word = best_result_end_word;
    if (input->cur_word == input->word_count) {
        input->next_cmd = 0;
    } else {
        input->next_cmd = input->cur_word;
    }

    for (int i = 0; i < PARSE_MAX_NOUNS; ++i) {
        if (input->nouns[i] && input->nouns[i]->ambig) {
            text_out("You'll need to be more specific whether you mean ");
            noun_t *cur = input->nouns[i];
            while (cur) {
                if (cur != input->nouns[i]) {
                    text_out(", ");
                    if (cur->ambig == NULL) {
                        text_out("or ");
                    }
                }
                object_name_print(gd, cur->object);
                cur = cur->ambig;
            }
            text_out(".\n");
            best_result = PARSE_AMBIG;
        }
    }

    input->action = -1;
    switch(best_result) {
        case PARSE_AMBIG:
//            printf("Multiple items matched.\n");
            break;
        case PARSE_BADNOUN:
            text_out("Not visible.\n");
            break;
        case PARSE_NONMATCH:
            text_out("Unrecognized verb '%s'.\n", input->words[input->next_cmd].word);
            break;
        case PARSE_BADTOKEN:
            text_out("Parser error.\n");
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
        text_out("FATAL: no gameinfo object\n");
        free_data(gd);
        return 0;
    }

    property_t *player_prop = object_property_get(gd->gameinfo, property_number(gd, "#player"));
    if (!player_prop || !player_prop->value.d.ptr) {
        text_out("FATAL: gameinfo does not define valid initial player object\n");
        free_data(gd);
        return 0;
    }
    gd->player = player_prop->value.d.ptr;

    text_out("if-parse-test VERSION %d.%d.%d\n\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
    text_out("--------------------------------------------------------------------------------\n\n");

    property_t *intro_prop = object_property_get(gd->gameinfo, property_number(gd, "#intro"));
    if (intro_prop && intro_prop->value.d.ptr) {
        text_out("%s\n", (char*)intro_prop->value.d.ptr);
    }

    return 1;
}

void input_free(input_t *input) {
    for (int i = 0; i < PARSE_MAX_NOUNS; ++i) {
        free_noun_list(input->nouns[i]);
    }
    free(input->input);
    free(input);
}

int main() {
    gamedata_t *gd = load_data();
    if (!gd) return 1;
    if (!game_init(gd)) return 1;

    input_t *input = NULL;
    print_location(gd, gd->player->parent);
    while (!gd->quit_game) {
        if (input == NULL) {
            input = calloc(sizeof(input_t), 1);
            text_out("\n> ");
            input->input = read_line();

            if (input->input[0] == '(') {
                list_t *list = parse_string(input->input);
                input_free(input);
                input = NULL;
                list_t *result = list_run(gd, NULL, list);
                text_out("RESULT: ");
                dump_list(stdout, result);
                text_out("\n");
                continue;
            }

            if (!tokenize(input)) {
                text_out("Pardon?\n");
                input_free(input);
                input = NULL;
                continue;
            }
        }
        if (!parse(gd, input)) {
            input_free(input);
            input = NULL;
            continue;
        }

        dispatch_action(gd, input);
        if (!input->next_cmd) {
            input_free(input);
            input = NULL;
        }
    }

    text_out("Goodbye!\n\n");
    free_data(gd);
    return 0;
}
