#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

void object_property_print(object_t *obj, int prop_num) {
    property_t *prop = object_property_get(obj, prop_num);
    if (prop) {
        printf("%s", (char*)prop->value.d.ptr);
    } else {
        printf("(obj#%d)", obj->id);
    }
}

void print_list_horz(object_t *parent_obj) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        if (obj != parent_obj->first_child) {
            printf(", ");
            if (!obj->sibling) {
                printf("and ");
            }
        }
        object_property_print(obj, PI_NAME);

        if (obj->first_child) {
            printf(" (containing ");
            print_list_horz(obj);
            printf(")");
        }

        obj = obj->sibling;
    }
}

void print_list_vert_core(object_t *parent_obj, int depth) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        for (int i = 0; i < depth; ++i) {
            printf("    ");
        }
        object_property_print(obj, PI_NAME);
        printf("\n");

        if (obj->first_child) {
            print_list_vert_core(obj, depth+1);
        }

        obj = obj->sibling;
    }
}

void print_list_vert(object_t *parent_obj) {
    print_list_vert_core(parent_obj, 1);
}

void print_location(object_t *location) {
    printf("\n**");
    object_property_print(location, PI_NAME);
    printf("**\n");
    object_property_print(location, PI_DESC);
    printf("\n");
    if (location->first_child) {
        printf("\nYou can see: ");
        print_list_horz(location);
        printf(".\n");
    }
}


void quit_sub(gamedata_t *gd) {
    gd->quit_game = 1;
}

void take_sub(gamedata_t *gd) {
    if (gd->objects[0] == gd->player) {
        printf("Cannot take yourself.\n");
    } else if (object_contains(gd->player, gd->objects[0])) {
        printf("Already taken.\n");
    } else {
        object_move(gd->objects[0], gd->player);

        property_t *p = object_property_get(gd->objects[0], PI_DESC);
        if (p) {
            for (int i = 0; i < p->value.array_size; ++i) {
                printf("%d: %d\n", i, ((value_t*)p->value.d.ptr)[i].d.num);
            }
        } else {
            printf("!P\n");
        }

        printf("Taken.\n");
    }
}

void drop_sub(gamedata_t *gd) {
    if (gd->objects[0] == gd->player) {
        printf("Cannot drop yourself.\n");
    } else if (!object_contains(gd->player, gd->objects[0])) {
        printf("Not carried.\n");
    } else {
        object_move(gd->objects[0], gd->player->parent);
        printf("Dropped.\n");
    }
}


void move_sub(gamedata_t *gd) {
    static struct {
        int word;
        int prop;
    } compass[8] = { { 0 } };

    if (compass[0].word == 0) {
        compass[0].word = vocab_index("north");
        compass[0].prop = property_number(gd, "north");
        compass[1].word = vocab_index("n");
        compass[1].prop = property_number(gd, "north");
        compass[2].word = vocab_index("south");
        compass[2].prop = property_number(gd, "south");
        compass[3].word = vocab_index("s");
        compass[3].prop = property_number(gd, "south");
        compass[4].word = vocab_index("east");
        compass[4].prop = property_number(gd, "east");
        compass[5].word = vocab_index("e");
        compass[5].prop = property_number(gd, "east");
        compass[6].word = vocab_index("west");
        compass[6].prop = property_number(gd, "west");
        compass[7].word = vocab_index("w");
        compass[7].prop = property_number(gd, "west");
    }

    int dir_prop = -1;
    for (int i = 0; i < 8; ++i) {
        if (gd->words[1].word_no == compass[i].word) {
            dir_prop = compass[i].prop;
            break;
        }
    }

    if (dir_prop == -1) {
        printf("Unknown direction.\n");
        return;
    }

    object_t *cur = gd->player->parent;
    property_t *prop = object_property_get(cur, dir_prop);
    if (prop) {
        if (prop->value.type == PT_OBJECT) {
            object_move(gd->player, prop->value.d.ptr);
            print_location(gd->player->parent);
        } else if (prop->value.type == PT_STRING) {
            printf("%s", prop->value.d.ptr);
        }
    } else {
            printf("Can't go that way.\n");
    }
}

void inv_sub(gamedata_t *gd) {
    object_t *cur = gd->player->first_child;
    if (!cur) {
        printf("You are carrying nothing.\n");
        return;
    }
    printf("You are carrying:\n");
    print_list_vert(gd->player);
}

void look_sub(gamedata_t *gd) {
    print_location(gd->player->parent);
}

void putin_sub(gamedata_t *gd) {
    if (gd->objects[0] == gd->player || gd->objects[1] == gd->player) {
        printf("Cannot put yourself somewhere.\n");
    } else if (object_contains(gd->objects[1], gd->objects[0])) {
        printf("Already there.\n");
    } else if (object_contains(gd->objects[0], gd->objects[1])) {
        printf("Not possible.\n");
    } else {
        object_move(gd->objects[0], gd->objects[1]);
        printf("Done.\n");
    }
    printf("# PUT ");
    object_property_print(gd->objects[0], PI_NAME);
    printf(" IN ");
    object_property_print(gd->objects[1], PI_NAME);
    printf(" #\n");
}

void (*cmd_dispatch[])(gamedata_t *gd) = {
    quit_sub,
    take_sub,
    drop_sub,
    move_sub,
    inv_sub,
    look_sub,
    putin_sub
};



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

object_t* match_noun(gamedata_t *gd, int *first_word) {
    object_t *obj, *obj_list[16], *match = NULL;
    int queue;

    obj = scope_ceiling(gd, gd->player);

    queue = 1;
    gd->search[0] = obj;
    gd->search_count = 1;
    obj_list[0] = obj->first_child;
    obj = obj->first_child;
    while (queue > 0) {
        --queue;
        object_t *here = obj_list[queue];
        gd->search[gd->search_count] = here;
        ++gd->search_count;

        if (here->sibling) {
            obj_list[queue] = here->sibling;
            ++queue;
        }
        if (here->first_child) {
            obj_list[queue] = here->first_child;
            ++queue;
        }
    }

    printf("finding noun for %s\n", gd->words[*first_word].word);
    for (int i = 0; i < gd->search_count; ++i) {
        printf("OBJECT ");
        object_property_print(gd->search[i], property_number(gd, "name"));
        printf("\n");

        if (word_in_property(gd->search[i],
                                property_number(gd, "vocab"),
                                gd->words[*first_word].word_no)) {
            printf("   (matched)\n");
            if (match) {
                printf("   (too many!)\n");
                return (object_t*)-1;
            } else {
                match = gd->search[i];
            }
        }
    }
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

        switch(action->grammar[token_a].type) {
            case GT_END:
                printf("PARSE ERROR: Encountered GT_END in grammar; this should have already been handled.\n");
                break;
            case GT_NOUN:
                obj = match_noun(gd, &gd->cur_word);
                if (!obj) {
                    printf("Not visible.\n");
                    return PARSE_NONMATCH;
                } else if (obj == (object_t*)-1) {
                    printf("Multiple items matched.\n");
                    gd->action = PARSE_AMBIG;
                } else {
                    gd->objects[gd->noun_count] = obj;
                    ++gd->noun_count;
                }
                ++token_a;
                ++gd->cur_word;
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

    gd->noun_count = 0;

    if (gd->words[0].word_no == -1) {
        printf("Unknown word '%s'.\n", gd->words[0].word);
        return 0;
    }

    while (cur_action) {
        int result = try_parse_action(gd, cur_action);
        if (result >= 0) {
            gd->action = result;
            break;
        }
        cur_action = cur_action->next;
    }

    return 1;
}

int main() {
    gamedata_t *gd = load_data();
    if (!gd) return 1;

//    objectloop_depth_first(gd->root, testfunc);

    gd->player = object_get_by_ident(gd, "player");
    print_location(gd->player->parent);
    while (!gd->quit_game) {
        get_line(gd->input, MAX_INPUT_LENGTH-1);

        if (!tokenize(gd) || !parse(gd)) {
            continue;
        }

        if (gd->action < 0) {
            printf("Unrecognized command.\n");
            continue;
        }

        cmd_dispatch[gd->action](gd);
    }

    printf("Goodbye!\n\n");
    free_data(gd);
    return 0;
}
