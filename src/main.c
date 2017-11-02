#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

void object_name_print(gamedata_t *gd, object_t *obj);
void object_property_print(object_t *obj, int prop_num);
void print_location(gamedata_t *gd, object_t *location);
int dispatch_action(gamedata_t *gd);


void testfunc(object_t *obj) {
    printf("obj #%02d :: %p -- parent %p -- first %p, sibling %p\n",
            obj->id,
            (void*)obj,
            (void*)obj->parent,
            (void*)obj->first_child,
            (void*)obj->sibling);
    property_t *cur = obj->properties;
    while (cur) {
        printf("    prop %02d (%d) ", cur->id, cur->value.type);
        switch(cur->value.type) {
            case PT_INTEGER:
                printf("%d", cur->value.d.num);
                break;
            case PT_OBJECT:
                printf("%p: #%d", cur->value.d.ptr, ((object_t*)cur->value.d.ptr)->id);
                break;
            case PT_STRING:
                printf("%p: ~%s~", cur->value.d.ptr, (char*)cur->value.d.ptr);
                break;
            case PT_ARRAY: {
                printf("%p: [%d]", cur->value.d.ptr, cur->value.array_size);
                value_t *arr = cur->value.d.ptr;
                for (int i = 0; i < cur->value.array_size; ++i) {
                    switch(arr[i].type) {
                        case PT_STRING: printf(" ~%s~", arr[i].d.ptr); break;
                        case PT_INTEGER: printf(" %d", arr[i].d.num); break;
                        case PT_OBJECT: printf(" (obj#%d)", ((object_t*)arr[i].d.ptr)->id); break;
                        default: printf(" (type#%d)", arr[i].type);
                    }
                }
                break; }

        }
        printf("\n");
        cur = cur->next;
    }
}


void get_line(char *buffer, int length) {
    printf("\n> ");
    fgets(buffer, length, stdin);
}

int tokenize(gamedata_t *gd) {
    int in_word = 0, count = 0;
    memset(gd->words, 0, sizeof(cmd_token_t) * MAX_INPUT_WORDS);
    for (int i = 0; i < MAX_INPUT_LENGTH; ++i) {
        unsigned char here = gd->input[i];
        if (isspace(here) || here == 0) {
            if (in_word) {
                in_word = 0;
                gd->words[count].end = i;
                gd->input[i] = 0;
                gd->words[count].word_no = vocab_index(gd->words[count].word);
                ++count;
            }
            if (here != 0) {
                continue;
            } else {
                break;
            }
        }
        if (!in_word) {
            gd->words[count].start = i;
            gd->words[count].word = &gd->input[i];
            in_word = 1;
        }
    }
    if (count == 0) {
        printf("Pardon?\n");
        return 0;
    } else {
        gd->words[count].word_no = -1;
    }

    return 1;
}

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

object_t* match_noun(gamedata_t *gd) {
    object_t *match = NULL;
    int match_strength = 0;
    int prop_vocab = property_number(gd, "vocab");
    printf("finding noun...\n");
    for (int i = 0; i < gd->search_count; ++i) {
        printf("   OBJECT ");
        object_property_print(gd->search[i], property_number(gd, "name"));

        int words = 0;
        int cur_word = gd->cur_word;
        while (word_in_property(gd->search[i], prop_vocab,
                                gd->words[cur_word].word_no)) {
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
            match = (object_t*)-1;
        } else {
            printf("\n");
        }
    }

    gd->cur_word += match_strength;
    return match;
}

int try_parse_action(gamedata_t *gd, action_t *action) {
    int token_a = 0;
    gd->cur_word = 0;
    object_t *obj;

    while (1) {
        if (action->grammar[token_a].type == GT_END) {
            if (!gd->words[gd->cur_word].word) {
                return action->action_code;
            } else {
                return PARSE_NONMATCH;
            }
        } else if (!gd->words[gd->cur_word].word) {
            return PARSE_NONMATCH;
        }

        gd->search_count = 0;
        switch(action->grammar[token_a].type) {
            case GT_END:
                printf("PARSE ERROR: Encountered GT_END in grammar; this should have already been handled.\n");
                break;
            case GT_NOUN:
                obj = scope_ceiling(gd, gd->player);
                add_to_scope(gd, obj);
                scope_within(gd, obj);
                obj = match_noun(gd);
                if (!obj) {
                    return token_a ? PARSE_BADNOUN : PARSE_NONMATCH;
                } else if (obj == (object_t*)-1) {
                    return PARSE_AMBIG;
                } else {
                    gd->objects[gd->noun_count] = obj;
                    ++gd->noun_count;
                }
                ++token_a;
                break;
            case GT_SCOPE:
                scope_within(gd, (object_t*)action->grammar[token_a].ptr);
                obj = match_noun(gd);
                if (!obj) {
                    return token_a ? PARSE_BADNOUN : PARSE_NONMATCH;
                } else if (obj == (object_t*)-1) {
                    return PARSE_AMBIG;
                } else {
                    gd->objects[gd->noun_count] = obj;
                    ++gd->noun_count;
                }
                ++token_a;
                break;
            case GT_ANY:
                ++token_a;
                ++gd->cur_word;
                break;
            case GT_WORD:
                if (gd->words[gd->cur_word].word_no == action->grammar[token_a].value) {
                    while (action->grammar[token_a].flags & GF_ALT) {
                        ++token_a;
                    }
                    ++token_a;
                    ++gd->cur_word;
                } else {
                    if (action->grammar[token_a].flags & GF_ALT) {
                        ++token_a;
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

int parse(gamedata_t *gd) {
    action_t *cur_action = gd->actions;
    int best_result = PARSE_BADTOKEN;
    gd->action = PARSE_BADTOKEN;
    gd->noun_count = 0;

    if (gd->words[0].word_no == -1) {
        printf("Unknown word '%s'.\n", gd->words[0].word);
        return 0;
    }

    while (cur_action && best_result < 0) {
        int result = try_parse_action(gd, cur_action);
        if (best_result < result) {
            best_result = result;
        }
        if (result >= 0) {
            break;
        }
        cur_action = cur_action->next;
    }

    switch(best_result) {
        case PARSE_AMBIG:
            printf("Multiple items matched.\n");
            break;
        case PARSE_BADNOUN:
            printf("Not visible.\n");
            break;
        case PARSE_NONMATCH:
            printf("Unrecognized verb '%s'.\n", gd->words[0].word);
            break;
        case PARSE_BADTOKEN:
            printf("Parser error.\n");
            break;
        default:
            gd->action = best_result;
    }

    return gd->action >= 0;
}

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

//    objectloop_depth_first(gd->root, testfunc);

    print_location(gd, gd->player->parent);
    while (!gd->quit_game) {
        get_line(gd->input, MAX_INPUT_LENGTH-1);

        if (!tokenize(gd) || !parse(gd)) {
            continue;
        }

        dispatch_action(gd);
    }

    printf("Goodbye!\n\n");
    free_data(gd);
    return 0;
}
