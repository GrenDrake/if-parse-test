#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

void object_name_print(gamedata_t *gd, object_t *obj);
void object_property_print(object_t *obj, int prop_num);
void print_list_horz(gamedata_t *gd, object_t *parent_obj);
void print_list_vert_core(gamedata_t *gd, object_t *parent_obj, int depth);
void print_list_vert(gamedata_t *gd, object_t *parent_obj);
void print_location(gamedata_t *gd, object_t *location);

int dispatch_action(gamedata_t *gd, input_t *input);

void quit_sub(gamedata_t *gd, input_t *input);
void take_sub(gamedata_t *gd, input_t *input);
void drop_sub(gamedata_t *gd, input_t *input);
void move_sub(gamedata_t *gd, input_t *input);
void inv_sub(gamedata_t *gd, input_t *input);
void look_sub(gamedata_t *gd, input_t *input);
void putin_sub(gamedata_t *gd, input_t *input);
void examine_sub(gamedata_t *gd, input_t *input);


/* ****************************************************************************
 * Utility methods
 * ****************************************************************************/
void object_name_print(gamedata_t *gd, object_t *obj) {
    int prop_article = property_number(gd, "article");
    property_t *article = object_property_get(obj, prop_article);
    if (article) {
        printf("%s", (char*)article->value.d.ptr);
        putchar(' ');
    } else {
        printf("a ");
    }
    object_property_print(obj, property_number(gd, "name"));
}

void object_property_print(object_t *obj, int prop_num) {
    property_t *prop = object_property_get(obj, prop_num);
    if (prop) {
        printf("%s", (char*)prop->value.d.ptr);
    } else {
        printf("(obj#%d)", obj->id);
    }
}

void print_list_horz(gamedata_t *gd, object_t *parent_obj) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        if (obj != parent_obj->first_child) {
            printf(", ");
            if (!obj->sibling) {
                printf("and ");
            }
        }
        object_name_print(gd, obj);

        if (obj->first_child) {
            printf(" (containing ");
            print_list_horz(gd, obj);
            printf(")");
        }

        obj = obj->sibling;
    }
}

void print_list_vert_core(gamedata_t *gd, object_t *parent_obj, int depth) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        for (int i = 0; i < depth; ++i) {
            printf("    ");
        }
        object_name_print(gd, obj);
        printf("\n");

        if (obj->first_child) {
            print_list_vert_core(gd, obj, depth+1);
        }

        obj = obj->sibling;
    }
}

void print_list_vert(gamedata_t *gd, object_t *parent_obj) {
    print_list_vert_core(gd, parent_obj, 1);
}

void print_location(gamedata_t *gd, object_t *location) {
    putchar('\n');
    style_bold();
    printf("**");
    object_property_print(location, PI_NAME);
    printf("**");
    style_normal();
    putchar('\n');
    object_property_print(location, PI_DESC);
    printf("\n");
    if (location->first_child) {
        printf("\nYou can see: ");
        print_list_horz(gd, location);
        printf(".\n");
    }
}


/* ****************************************************************************
 * Verb dispatching
 * ****************************************************************************/
int dispatch_action(gamedata_t *gd, input_t *input) {
    switch(input->action) {
        case ACT_QUIT:
            quit_sub(gd, input);
            return 1;
        case ACT_TAKE:
            take_sub(gd, input);
            return 1;
        case ACT_DROP:
            drop_sub(gd, input);
            return 1;
        case ACT_MOVE:
            move_sub(gd, input);
            return 1;
        case ACT_INVENTORY:
            inv_sub(gd, input);
            return 1;
        case ACT_LOOK:
            look_sub(gd, input);
            return 1;
        case ACT_PUTIN:
            putin_sub(gd, input);
            return 1;
        case ACT_EXAMINE:
            examine_sub(gd, input);
            return 1;
        default:
            printf("Unhandled action #%d (%s)\n",
                    input->action, input->words[0].word);
            return 0;
    }
}


/* ****************************************************************************
 * Verb methods
 * ****************************************************************************/
void quit_sub(gamedata_t *gd, input_t *input) {
    gd->quit_game = 1;
}

void take_sub(gamedata_t *gd, input_t *input) {
    if (input->nouns[0] == gd->player) {
        printf("Cannot take yourself.\n");
    } else if (object_contains(gd->player, input->nouns[0])) {
        printf("Already taken.\n");
    } else {
        property_t *p = object_property_get(input->nouns[0], property_number(gd, "is_takable"));
        if (p && p->value.type == PT_INTEGER && p->value.d.num == 0) {
            printf("Impossible.\n");
        } else {
            object_move(input->nouns[0], gd->player);
            printf("Taken.\n");
        }
    }
}

void drop_sub(gamedata_t *gd, input_t *input) {
    if (input->nouns[0] == gd->player) {
        printf("Cannot drop yourself.\n");
    } else if (!object_contains(gd->player, input->nouns[0])) {
        printf("Not carried.\n");
    } else {
        object_move(input->nouns[0], gd->player->parent);
        printf("Dropped.\n");
    }
}


void move_sub(gamedata_t *gd, input_t *input) {
    property_t *p = object_property_get(input->nouns[0], property_number(gd, "dir_prop"));
    if (!p || p->value.type != PT_STRING) {
        printf("Malformed direction.\n");
        return;
    }

    property_t *prop = object_property_get(gd->player->parent, property_number(gd, (char*)p->value.d.ptr));
    if (prop) {
        if (prop->value.type == PT_OBJECT) {
            object_move(gd->player, prop->value.d.ptr);
            print_location(gd, gd->player->parent);
        } else if (prop->value.type == PT_STRING) {
            printf("%s", prop->value.d.ptr);
        }
    } else {
        printf("Can't go that way.\n");
    }
}

void inv_sub(gamedata_t *gd, input_t *input) {
    object_t *cur = gd->player->first_child;
    if (!cur) {
        printf("You are carrying nothing.\n");
        return;
    }
    if (input->words[1].word && input->words[1].word_no == vocab_index("wide")) {
        printf("You are carrying: ");
        print_list_horz(gd, gd->player);
        putchar('\n');
    } else {
        printf("You are carrying:\n");
        print_list_vert(gd, gd->player);
    }
}

void look_sub(gamedata_t *gd, input_t *input) {
    print_location(gd, gd->player->parent);
}

void putin_sub(gamedata_t *gd, input_t *input) {
    if (input->nouns[0] == gd->player || input->nouns[1] == gd->player) {
        printf("Cannot put yourself somewhere.\n");
    } else if (object_contains(input->nouns[1], input->nouns[0])) {
        printf("Already there.\n");
    } else if (object_contains(input->nouns[0], input->nouns[1])) {
        printf("Not possible.\n");
    } else {
        property_t *p = object_property_get(input->nouns[1], property_number(gd, "is_container"));
        if (!p || p->value.type != PT_INTEGER || p->value.d.num == 0) {
            printf("That can't contain things.\n");
            return;
        }
        p = object_property_get(input->nouns[1], property_number(gd, "is_open"));
        if (p && p->value.type == PT_INTEGER && p->value.d.num == 0) {
            printf("It's not open.\n");
            return;
        }
        object_move(input->nouns[0], input->nouns[1]);
        printf("Done.\n");
    }
}

void examine_sub(gamedata_t *gd, input_t *input) {
    property_t *p = object_property_get(input->nouns[0], property_number(gd, "description"));
    if (p && p->value.type == PT_STRING) {
        printf("%s\n", (void*)p->value.d.ptr);
    } else {
        printf("It looks as expected.\n");
    }
}
