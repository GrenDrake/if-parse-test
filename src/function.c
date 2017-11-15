#include <stdio.h>
#include <string.h>

#include "parse.h"

typedef struct FUNCDEF {
    const char *name;
    list_t* (*func)(gamedata_t *gd, list_t *args);
} funcdef_t;

list_t *list_run_function(gamedata_t *gd, function_t *func, list_t *args);
list_t *list_evaluate(gamedata_t *gd, symboltable_t *locals, list_t *list);

static list_t* builtin_add(gamedata_t *gd, list_t *args);
static list_t* builtin_sub(gamedata_t *gd, list_t *args);
static list_t* builtin_mul(gamedata_t *gd, list_t *args);
static list_t* builtin_div(gamedata_t *gd, list_t *args);
static list_t* builtin_dump_symbols(gamedata_t *gd, list_t *args);

static funcdef_t builtin_funcs[] = {
    { "add", builtin_add },
    { "sub", builtin_sub },
    { "mul", builtin_mul },
    { "div", builtin_div },
    { "dump-symbols", builtin_dump_symbols },
    { NULL }
};

list_t *list_run_function(gamedata_t *gd, function_t *func, list_t *args) {
    symboltable_t *locals = symboltable_create();

    list_t *result = list_run(gd, locals, func->body);

    symboltable_free(locals);
    return result;
}

list_t *list_evaluate(gamedata_t *gd, symboltable_t *locals, list_t *list) {
    symbol_t *symbol = NULL;
    list_t *new_list;
    switch(list->type) {
        case T_STRING:
        case T_INTEGER:
        case T_VOCAB:
            return list_duplicate(list);
        case T_ATOM:
            if (locals) {
                symbol = symbol_get(locals, list->text);
            }
            if (!symbol) {
                symbol = symbol_get(gd->symbols, list->text);
            }
            if (!symbol) {
                debug_out("undefined value %s\n", list->text);
                return list_create_false();
            }
            switch(symbol->type) {
                case SYM_PROPERTY:
                case SYM_CONSTANT:
                    new_list = list_create();
                    new_list->type = T_INTEGER;
                    new_list->number = symbol->d.value;
                    return new_list;
                default:
                    debug_out("list atom evaluated to unhandled type %d\n", symbol->type);
                    return list_create_false();
            }
            break;
        case T_LIST:
            return list_run(gd, locals, list);
        default:
            debug_out("Tried to evaluate list of unknown type %d\n", list->type);
            return list_create_false();
    }
}

list_t *list_run(gamedata_t *gd, symboltable_t *locals, list_t *list) {
    if (!gd || !list) {
        return NULL;
    }
    if (list->child->type != T_ATOM) {
        debug_out("Tried to run list, but list did not start with atom.\n");
        return NULL;
    }
    const char *name = list->child->text;

    list_t *args = list_create();
    list_t *iter = list->child->next;
    while (iter) {
        list_add(args, list_evaluate(gd, locals, iter));
        iter = iter->next;
    }

    symbol_t *user_func = symbol_get(gd->symbols, name);
    if (user_func) {
        if (user_func->type != SYM_FUNCTION) {
            debug_out("tried to run non-function %s\n", name);
            return list_create_false();
        }
        return list_run_function(gd, (function_t*)user_func->d.ptr, args);
    }

    int result = -1;
    for (int i = 0; builtin_funcs[i].name != NULL && result == -1; ++i) {
        if (strcmp(builtin_funcs[i].name, name) == 0) {
            result = i;
        }
    }
    if (result == -1) {
        debug_out("tried to run non-existant function %s\n", name);
        list_free(args);
        return list_create_false();
    }

    list_t *run_result = builtin_funcs[result].func(gd, args);
    list_free(args);
    return run_result;
}

/* *********************************************************************** *
* Functions for built-in scipt functions
 * *********************************************************************** */


list_t* builtin_add(gamedata_t *gd, list_t *args) {
    int total = 0;
    list_t *iter = args->child;
    while (iter) {
        if (iter->type != T_INTEGER) {
            debug_out("add: requires integer arguments\n");
            return list_create_false();
        }
        total += iter->number;
        iter = iter->next;
    }
    list_t *result = list_create();
    result->type = T_INTEGER;
    result->number = total;
    return result;
}

list_t* builtin_sub(gamedata_t *gd, list_t *args) {
    int total = 0;
    list_t *iter = args->child;
    if (iter) {
        if (iter->type != T_INTEGER) {
            debug_out("sub: requires integer arguments\n");
            return list_create_false();
        }
        total = iter->number;
        iter = iter->next;

        while (iter) {
            if (iter->type != T_INTEGER) {
                debug_out("sub: requires integer arguments\n");
                return list_create_false();
            }
            total -= iter->number;
            iter = iter->next;
        }
    }

    list_t *result = list_create();
    result->type = T_INTEGER;
    result->number = total;
    return result;
}

list_t* builtin_mul(gamedata_t *gd, list_t *args) {
    int total = 1;
    list_t *iter = args->child;
    while (iter) {
        if (iter->type != T_INTEGER) {
            debug_out("mul: requires integer arguments\n");
            return list_create_false();
        }
        total *= iter->number;
        iter = iter->next;
    }
    list_t *result = list_create();
    result->type = T_INTEGER;
    result->number = total;
    return result;
}

list_t* builtin_div(gamedata_t *gd, list_t *args) {
    int total = 0;
    list_t *iter = args->child;
    if (iter) {
        if (iter->type != T_INTEGER) {
            debug_out("div: requires integer arguments\n");
            return list_create_false();
        }
        total = iter->number;
        iter = iter->next;

        while (iter) {
            if (iter->type != T_INTEGER) {
                debug_out("div: requires integer arguments\n");
                return list_create_false();
            }
            total /= iter->number;
            iter = iter->next;
        }
    }

    list_t *result = list_create();
    result->type = T_INTEGER;
    result->number = total;
    return result;
}

list_t* builtin_dump_symbols(gamedata_t *gd, list_t *args) {
    dump_symbol_table(stdout, gd);
    return list_create_false();
}
