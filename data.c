#include <stdlib.h>
#include <string.h>

#include "parse.h"

const char *vocab[] = {
    "apricot",
    "drop",
    "e",
    "east",
    "get",
    "go",
    "i",
    "in",
    "inv",
    "inventory",
    "l",
    "look",
    "me",
    "move",
    "n",
    "north",
    "on",
    "player",
    "put",
    "q",
    "quit",
    "s",
    "south",
    "take",
    "turnip",
    "w",
    "walk",
    "west",
    "umbrella",
    0
};

gamedata_t* parse_file(const char *filename);

gamedata_t *gamedata_create() {
    gamedata_t *gd = calloc(sizeof(gamedata_t), 1);
    gd->root = object_create(NULL);
    return gd;
}

gamedata_t* load_data() {
    action_t *act;
//    object_t *entryway, *kitchen, *bedroom, *obj;

    gamedata_t *gd = parse_file("game.dat");
    if (!gd) {
        printf("Error loading game data.\n");
        return NULL;
    }

    // gd->root = object_create(NULL);

    // entryway = object_create(gd->root);
    // object_property_add_string(entryway, PI_NAME, "Entryway");
    // obj = object_create(entryway);
    // object_property_add_string(obj, PI_NAME, "table");
    // obj = object_create(entryway);
    // object_property_add_string(obj, PI_NAME, "chair");
    // obj = object_create(entryway);
    // object_property_add_string(obj, PI_NAME, "painting");
    // obj = object_create(entryway);
    // object_property_add_string(obj, PI_NAME, "fireplace");

    // kitchen = object_create(gd->root);
    // object_property_add_string(kitchen, PI_NAME, "Kitchen");
    // object_property_add_string(kitchen, PI_DESC, "A well appointed kitchen, suitable for even the finest tastes.");
    // obj = object_create(kitchen);
    // object_property_add_string(obj, PI_NAME, "apricot");
    // object_property_add_integer(obj, PI_VOCAB, vocab_index("apricot"));
    // obj = object_create(kitchen);
    // object_property_add_string(obj, PI_NAME, "turnip");
    // object_property_add_integer(obj, PI_VOCAB, vocab_index("turnip"));
    // object_property_add_array(obj, PI_DESC, 3);
    // property_t *prop = object_property_get(obj, PI_DESC);
    // ((value_t*)prop->value.d.ptr)[0].type = PT_INTEGER;
    // ((value_t*)prop->value.d.ptr)[0].d.num = 4;
    // ((value_t*)prop->value.d.ptr)[1].type = PT_INTEGER;
    // ((value_t*)prop->value.d.ptr)[1].d.num = 543;

    // bedroom = object_create(gd->root);
    // object_property_add_string(bedroom, PI_NAME, "Bedroom");
    // object_property_add_string(bedroom, PI_DESC, "A simple bedroom with a cot visible.");


    // object_property_add_object(entryway, PI_NORTH, kitchen);
    // object_property_add_object(kitchen, PI_NORTH, bedroom);
    // object_property_add_object(bedroom, PI_NORTH, entryway);


    // gd->player = object_create(kitchen);
    // object_property_add_string(gd->player, PI_NAME, "yourself");
    // object_property_add_integer(gd->player, PI_VOCAB, vocab_index("me"));
    // obj = object_create(gd->player);
    // object_property_add_string(obj, PI_NAME, "clear umbrella");
    // object_property_add_integer(obj, PI_VOCAB, vocab_index("umbrella"));
    // obj = object_create(gd->player);
    // object_property_add_string(obj, PI_NAME, "black umbrella");
    // object_property_add_integer(obj, PI_VOCAB, vocab_index("umbrella"));

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].flags = GF_ALT;
    act->grammar[0].value = vocab_index("quit");
    act->grammar[1].type = GT_WORD;
    act->grammar[1].value = vocab_index("q");
    act->action_code = ACT_QUIT;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].flags = GF_ALT;
    act->grammar[0].value = vocab_index("move");
    act->grammar[1].type = GT_WORD;
    act->grammar[1].flags = GF_ALT;
    act->grammar[1].value = vocab_index("go");
    act->grammar[2].type = GT_WORD;
    act->grammar[2].value = vocab_index("walk");
    act->grammar[3].type = GT_ANY;
    act->action_code = ACT_MOVE;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].flags = GF_ALT;
    act->grammar[0].value = vocab_index("i");
    act->grammar[1].type = GT_WORD;
    act->grammar[1].flags = GF_ALT;
    act->grammar[1].value = vocab_index("inv");
    act->grammar[2].type = GT_WORD;
    act->grammar[2].value = vocab_index("inventory");
    act->action_code = ACT_INVENTORY;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].value = vocab_index("drop");
    act->grammar[1].type = GT_NOUN;
    act->action_code = ACT_DROP;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].flags = GF_ALT;
    act->grammar[0].value = vocab_index("take");
    act->grammar[1].type = GT_WORD;
    act->grammar[1].value = vocab_index("get");
    act->grammar[2].type = GT_NOUN;
    act->action_code = ACT_TAKE;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].value = vocab_index("put");
    act->grammar[1].type = GT_NOUN;
    act->grammar[2].type = GT_WORD;
    act->grammar[2].flags = GF_ALT;
    act->grammar[2].value = vocab_index("on");
    act->grammar[3].type = GT_WORD;
    act->grammar[3].value = vocab_index("in");
    act->grammar[4].type = GT_NOUN;
    act->action_code = ACT_PUTIN;
    action_add(gd, act);

    act = calloc(sizeof(action_t), 1);
    act->grammar[0].type = GT_WORD;
    act->grammar[0].flags = GF_ALT;
    act->grammar[0].value = vocab_index("l");
    act->grammar[1].type = GT_WORD;
    act->grammar[1].value = vocab_index("look");
    act->action_code = ACT_LOOK;
    action_add(gd, act);

    return gd;
}

void free_data(gamedata_t *gd) {
    objectloop_free(gd->root);

    while (gd->actions) {
        action_t *next = gd->actions->next;
        free(gd->actions);
        gd->actions = next;
    }
}

object_t *object_get_by_ident(gamedata_t *gd, const char *ident) {
    object_t *obj = gd->root->first_child;
    while (obj) {
        property_t *prop = object_property_get(obj, PI_IDENT);
        if (prop) {
            if (prop->value.type == PT_STRING && strcmp(prop->value.d.ptr, ident)) {
                return obj;
            }
        }
        obj = obj->sibling;
    }
    return NULL;
}