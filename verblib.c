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

int dispatch_action(gamedata_t *gd);

void quit_sub(gamedata_t *gd);
void take_sub(gamedata_t *gd);
void drop_sub(gamedata_t *gd);
void move_sub(gamedata_t *gd);
void inv_sub(gamedata_t *gd);
void look_sub(gamedata_t *gd);
void putin_sub(gamedata_t *gd);
void examine_sub(gamedata_t *gd);


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
    printf("\n**");
    object_property_print(location, PI_NAME);
    printf("**\n");
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
int dispatch_action(gamedata_t *gd) {
    switch(gd->action) {
        case ACT_QUIT:
            quit_sub(gd);
            return 1;
        case ACT_TAKE:
            take_sub(gd);
            return 1;
        case ACT_DROP:
            drop_sub(gd);
            return 1;
        case ACT_MOVE:
            move_sub(gd);
            return 1;
        case ACT_INVENTORY:
            inv_sub(gd);
            return 1;
        case ACT_LOOK:
            look_sub(gd);
            return 1;
        case ACT_PUTIN:
            putin_sub(gd);
            return 1;
        case ACT_EXAMINE:
            examine_sub(gd);
            return 1;
        default: 
            printf("Unhandled action #%d (%s)\n",
                   gd->action, gd->words[0].word);
            return 0;
    }
}


/* ****************************************************************************
 * Verb methods
 * ****************************************************************************/
void quit_sub(gamedata_t *gd) {
    gd->quit_game = 1;
}

void take_sub(gamedata_t *gd) {
    if (gd->objects[0] == gd->player) {
        printf("Cannot take yourself.\n");
    } else if (object_contains(gd->player, gd->objects[0])) {
        printf("Already taken.\n");
    } else {
        property_t *p = object_property_get(gd->objects[0], property_number(gd, "is_takable"));
        if (p && p->value.type == PT_INTEGER && p->value.d.num == 0) {
            printf("Impossible.\n");
        } else {
            object_move(gd->objects[0], gd->player);
            printf("Taken.\n");
        }
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
            print_location(gd, gd->player->parent);
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
    if (gd->words[1].word && gd->words[1].word_no == vocab_index("wide")) {
        printf("You are carrying: ");
        print_list_horz(gd, gd->player);
        putchar('\n');
    } else {
        printf("You are carrying:\n");
        print_list_vert(gd, gd->player);
    }
}

void look_sub(gamedata_t *gd) {
    print_location(gd, gd->player->parent);
}

void putin_sub(gamedata_t *gd) {
    if (gd->objects[0] == gd->player || gd->objects[1] == gd->player) {
        printf("Cannot put yourself somewhere.\n");
    } else if (object_contains(gd->objects[1], gd->objects[0])) {
        printf("Already there.\n");
    } else if (object_contains(gd->objects[0], gd->objects[1])) {
        printf("Not possible.\n");
    } else {
        property_t *p = object_property_get(gd->objects[1], property_number(gd, "is_container"));
        if (!p || p->value.type != PT_INTEGER || p->value.d.num == 0) {
            printf("That can't contain things.\n");
            return;
        }
        p = object_property_get(gd->objects[1], property_number(gd, "is_open"));
        if (p && p->value.type == PT_INTEGER && p->value.d.num == 0) {
            printf("It's not open.\n");
            return;
        }
        object_move(gd->objects[0], gd->objects[1]);
        printf("Done.\n");
    }
}

void examine_sub(gamedata_t *gd) {
    property_t *p = object_property_get(gd->objects[0], property_number(gd, "description"));
    if (p && p->value.type == PT_STRING) {
        printf("%s\n", (void*)p->value.d.ptr);
    } else {
        printf("It looks as expected.\n");
    }
}
