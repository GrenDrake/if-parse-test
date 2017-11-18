#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

int dispatch_action(gamedata_t *gd, input_t *input);

void quit_sub(gamedata_t *gd, input_t *input);
void take_sub(gamedata_t *gd, input_t *input);
void drop_sub(gamedata_t *gd, input_t *input);
void move_sub(gamedata_t *gd, input_t *input);
void inv_sub(gamedata_t *gd, input_t *input);
void look_sub(gamedata_t *gd, input_t *input);
void putin_sub(gamedata_t *gd, input_t *input);
void examine_sub(gamedata_t *gd, input_t *input);
void dumpobj_sub(gamedata_t *gd, input_t *input);

/* ****************************************************************************
 * Utility methods
 * ****************************************************************************/
void object_name_print(gamedata_t *gd, object_t *obj) {
    int prop_isproper = property_number(gd, "#is-proper");
    int prop_name = property_number(gd, "#name");

    if (!object_property_is_true(obj, prop_isproper, 0)) {
        int prop_article = property_number(gd, "#article");
        property_t *article = object_property_get(obj, prop_article);
        if (article) {
            text_out("%s", (char*)article->value.d.ptr);
            putchar(' ');
        } else {
            property_t *name = object_property_get(obj, prop_name);
            int first_letter = 0;
            if (name && name->value.d.ptr) {
                first_letter = ((char*)name->value.d.ptr)[0];
            }

            if (first_letter == 'a' || first_letter == 'u' || first_letter == 'i' || first_letter == 'e' || first_letter == 'o') {
                text_out("an ");
            } else {
                text_out("a ");
            }
        }
    }
    object_property_print(obj, prop_name);
}

void object_property_print(object_t *obj, int prop_num) {
    property_t *prop = object_property_get(obj, prop_num);
    if (prop) {
        text_out("%s", (char*)prop->value.d.ptr);
    } else {
        text_out("(obj#%d)", obj->id);
    }
}

void print_list_horz(gamedata_t *gd, object_t *parent_obj) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        if (obj != parent_obj->first_child) {
            text_out(", ");
            if (!obj->sibling) {
                text_out("and ");
            }
        }
        object_name_print(gd, obj);

        if (obj->first_child) {
            text_out(" (containing ");
            print_list_horz(gd, obj);
            text_out(")");
        }

        obj = obj->sibling;
    }
}

void print_list_vert_core(gamedata_t *gd, object_t *parent_obj, int depth) {
    object_t *obj = parent_obj->first_child;
    while (obj) {
        for (int i = 0; i < depth; ++i) {
            text_out("    ");
        }
        object_name_print(gd, obj);
        text_out("\n");

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
    text_out("**");
    object_property_print(location, property_number(gd, "#name"));
    text_out("**");
    style_normal();
    putchar('\n');
    object_property_print(location, property_number(gd, "#description"));
    text_out("\n");
    if (location->first_child) {
        text_out("\nYou can see: ");
        print_list_horz(gd, location);
        text_out(".\n");
    }
}


/* ****************************************************************************
 * Verb dispatching
 * ****************************************************************************/
int dispatch_action(gamedata_t *gd, input_t *input) {
    switch(input->action) {
        case 9999:
            {
                function_t *func = input->action_func;
                list_t *args = list_create();

                for (int i = 0; i < PARSE_MAX_NOUNS; ++i) {
                    if (input->nouns[i]) {
                    list_t *noun = list_create();
                    noun->type = T_OBJECT_REF;
                        noun->ptr = input->nouns[i]->object;
                    list_add(args, noun);
                } else {
                        list_add(args, list_create_false());
                    }
                }
                list_run_function(gd, func, args);
            }
            return 1;
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
//            examine_sub(gd, input);
            return 1;
        case ACT_DUMPOBJ:
            dumpobj_sub(gd, input);
            return 1;
        default:
        text_out("Unhandled action #%d (%s)\n",
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
    if (input->nouns[0]->object == gd->player) {
        text_out("Cannot take yourself.\n");
    } else if (object_contains(gd->player, input->nouns[0]->object)) {
        text_out("Already taken.\n");
    } else {
        if (!object_property_is_true(input->nouns[0]->object, property_number(gd, "#is-takable"), 1)) {
            text_out("Impossible.\n");
        } else {
            object_move(input->nouns[0]->object, gd->player);
            text_out("Taken.\n");
        }
    }
}

void drop_sub(gamedata_t *gd, input_t *input) {
    if (input->nouns[0]->object == gd->player) {
        text_out("Cannot drop yourself.\n");
    } else if (!object_contains(gd->player, input->nouns[0]->object)) {
        text_out("Not carried.\n");
    } else {
        object_move(input->nouns[0]->object, gd->player->parent);
        text_out("Dropped.\n");
    }
}


void move_sub(gamedata_t *gd, input_t *input) {
    property_t *p = object_property_get(input->nouns[0]->object, property_number(gd, "#dir-prop"));
    if (!p || p->value.type != PT_STRING) {
        text_out("Malformed direction.\n");
        return;
    }

    property_t *prop = object_property_get(gd->player->parent, property_number(gd, (char*)p->value.d.ptr));
    if (prop) {
        if (prop->value.type == PT_OBJECT) {
            object_move(gd->player, prop->value.d.ptr);
            print_location(gd, gd->player->parent);
        } else if (prop->value.type == PT_STRING) {
            text_out("%s", (char*)prop->value.d.ptr);
        }
    } else {
        text_out("Can't go that way.\n");
    }
}

void inv_sub(gamedata_t *gd, input_t *input) {
    object_t *cur = gd->player->first_child;
    if (!cur) {
        text_out("You are carrying nothing.\n");
        return;
    }
    if (input->words[1].word && input->words[1].word_no == vocab_index("wide")) {
        text_out("You are carrying: ");
        print_list_horz(gd, gd->player);
        putchar('\n');
    } else {
        text_out("You are carrying:\n");
        print_list_vert(gd, gd->player);
    }
}

void look_sub(gamedata_t *gd, input_t *input) {
    print_location(gd, gd->player->parent);
}

void putin_sub(gamedata_t *gd, input_t *input) {
    if (input->nouns[0]->object == gd->player || input->nouns[1]->object == gd->player) {
        text_out("Cannot put yourself somewhere.\n");
    } else if (object_contains(input->nouns[1]->object, input->nouns[0]->object)) {
        text_out("Already there.\n");
    } else if (object_contains(input->nouns[0]->object, input->nouns[1]->object)) {
        text_out("Not possible.\n");
    } else {
//        property_t *p = object_property_get(input->nouns[1]->object, property_number(gd, "#is-container"));
//        if (!p || p->value.type != PT_INTEGER || p->value.d.num == 0) {
        if (!object_property_is_true(input->nouns[1]->object, property_number(gd, "#is-container"), 0)) {
            text_out("That can't contain things.\n");
            return;
        }
//        p = object_property_get(input->nouns[1]->object, property_number(gd, "#is-open"));
//        if (p && p->value.type == PT_INTEGER && p->value.d.num == 0) {
        if (!object_property_is_true(input->nouns[1]->object, property_number(gd, "#is-open"), 1)) {
            text_out("It's not open.\n");
            return;
        }
        object_move(input->nouns[0]->object, input->nouns[1]->object);
        text_out("Done.\n");
    }
}

void examine_sub(gamedata_t *gd, input_t *input) {
    property_t *p = object_property_get(input->nouns[0]->object, property_number(gd, "#description"));
    if (p && p->value.type == PT_STRING) {
        text_out("%s\n", (char*)p->value.d.ptr);
    } else {
        text_out("It looks as expected.\n");
    }
}

void dumpobj_sub(gamedata_t *gd, input_t *input) {
    object_dump(gd, input->nouns[0]->object);
}
