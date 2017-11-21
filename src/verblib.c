#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

int dispatch_action(gamedata_t *gd, input_t *input);

void quit_sub(gamedata_t *gd, input_t *input);
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

void print_location(gamedata_t *gd, object_t *location) {
    symbol_t *func_sym = symbol_get(gd->symbols, "print-location");
    if (!func_sym) return;
    function_t *func = func_sym->d.ptr;
    list_run_function_noargs(gd, func);
}


/* ****************************************************************************
 * Verb dispatching
 * ****************************************************************************/
int dispatch_action(gamedata_t *gd, input_t *input) {
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
    list_free(args);
    return 1;
}
