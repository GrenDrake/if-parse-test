#include <stdio.h>
#include <string.h>

#include "parse.h"

typedef struct FUNCDEF {
    const char *name;
    int auto_evaluate;
    list_t* (*func)(gamedata_t *gd, symboltable_t *locals, list_t *args);
} funcdef_t;

list_t *list_run_function(gamedata_t *gd, function_t *func, list_t *args);
list_t *list_evaluate(gamedata_t *gd, symboltable_t *locals, list_t *list);
static list_t *list_build_args_from(gamedata_t *gd, symboltable_t *locals, list_t *src, int evaluate);
static int is_defined(gamedata_t *gd, symboltable_t *locals, list_t *atom);

static list_t* builtin_add(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_sub(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_mul(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_div(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_dump_symbols(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_vocab(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_log(gamedata_t *gd, symboltable_t *locals, list_t *args);
static void builtin_log_helper(gamedata_t *gd, symboltable_t *locals, list_t *list);
static list_t* builtin_say(gamedata_t *gd, symboltable_t *locals, list_t *args);


static funcdef_t builtin_funcs[] = {
    { "add", TRUE, builtin_add },
    { "sub", TRUE, builtin_sub },
    { "mul", TRUE, builtin_mul },
    { "div", TRUE, builtin_div },
    { "dump-symbols", TRUE, builtin_dump_symbols },
    { "vocab", TRUE, builtin_vocab },
    { "log", FALSE, builtin_log },
    { "say", TRUE, builtin_say },
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

list_t *list_build_args_from(gamedata_t *gd, symboltable_t *locals, list_t *src, int evaluate) {
    list_t *args = list_create();
    list_t *iter = src;
    while (iter) {
        if (evaluate) {
            list_add(args, list_evaluate(gd, locals, iter));
        } else {
            list_add(args, list_duplicate(iter));
        }
        iter = iter->next;
    }
    return args;
}

int is_defined(gamedata_t *gd, symboltable_t *locals, list_t *atom) {
    symbol_t *symbol = NULL;
    if (!atom || atom->type != T_ATOM) {
        return FALSE;
    }
    if (locals) {
        symbol = symbol_get(locals, atom->text);
    }
    if (!symbol) {
        symbol = symbol_get(gd->symbols, atom->text);
    }
    if (!symbol) {
        return FALSE;
    }
    return TRUE;
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

    symbol_t *user_func = symbol_get(gd->symbols, name);
    if (user_func) {
        if (user_func->type != SYM_FUNCTION) {
            debug_out("tried to run non-function %s\n", name);
            return list_create_false();
        }
        list_t *args = list_build_args_from(gd, locals, list->child->next, TRUE);
        list_t *result = list_run_function(gd, (function_t*)user_func->d.ptr, args);
        list_free(args);
        return result;
    }

    int result = -1;
    for (int i = 0; builtin_funcs[i].name != NULL && result == -1; ++i) {
        if (strcmp(builtin_funcs[i].name, name) == 0) {
            result = i;
        }
    }
    if (result == -1) {
        debug_out("tried to run non-existant function %s\n", name);
        return list_create_false();
    }

    list_t *args = list_build_args_from(gd, locals, list->child->next, builtin_funcs[result].auto_evaluate);
    list_t *run_result = builtin_funcs[result].func(gd, locals, args);
    list_free(args);
    return run_result;
}

/* *********************************************************************** *
* Functions for built-in scipt functions
 * *********************************************************************** */


list_t* builtin_add(gamedata_t *gd, symboltable_t *locals, list_t *args) {
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

list_t* builtin_sub(gamedata_t *gd, symboltable_t *locals, list_t *args) {
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

list_t* builtin_mul(gamedata_t *gd, symboltable_t *locals, list_t *args) {
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

list_t* builtin_div(gamedata_t *gd, symboltable_t *locals, list_t *args) {
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

list_t* builtin_dump_symbols(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    dump_symbol_table(stdout, gd);
    return list_create_false();
}

list_t* builtin_vocab(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    vocab_dump();
    return list_create_false();
}

list_t* builtin_log(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    debug_out("builtin_log:");
    if (!args || !args->child) {
        debug_out(" NULL\n");
        return list_create_false();
    }
    list_t *list = args->child;
    while (list) {
        builtin_log_helper(gd, locals, list);
        list = list->next;
    }
    debug_out("\n");
    return list_create_false();
}
void builtin_log_helper(gamedata_t *gd, symboltable_t *locals, list_t *list) {
    list_t *result = NULL;
    switch(list->type) {
        case T_LIST:
            debug_out(" {");
            list_t *child = list->child;
            while (child) {
                builtin_log_helper(gd, locals, child);
                child = child->next;
            }
            debug_out(" }");
            break;
        case T_ATOM:
            debug_out(" %s =", list->text);
            if (is_defined(gd, locals, list)) {
                result = list_evaluate(gd, locals, list);
                builtin_log_helper(gd, locals, result);
            } else {
                debug_out(" NULL");
            }
            break;
        case T_STRING:
            debug_out(" %s", list->text);
            break;
        case T_INTEGER:
            debug_out(" %d", list->number);
            break;
        default:
            debug_out(" [unsupported list type %d]", list->type);
    }
}

list_t* builtin_say(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args || !args->child) {
        return list_create_false();
    }
    list_t *list = args->child;
    while (list) {
        switch(list->type) {
            case T_STRING:
                text_out("%s", list->text);
                break;
            case T_INTEGER:
                text_out("%d", list->number);
                break;
            default:
                text_out("[unsupported type %d]", list->type);
        }
        list = list->next;
    }
    return list_create_false();
}

