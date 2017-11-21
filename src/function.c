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
static list_t* builtin_list(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_quote(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_if(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_prop_has(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_prop_get(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_proc(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_parent(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_bold(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_normal(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_reverse(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_prop_true(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_or(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_and(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_not(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_sibling(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_child(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_set(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_object_move(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_contains(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_contains_indirect(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_eq(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_is_object(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_is_string(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_is_number(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_is_function(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_is_list(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_type_name(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_prop_set(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_request_quit(gamedata_t *gd, symboltable_t *locals, list_t *args);
static list_t* builtin_dump_obj(gamedata_t *gd, symboltable_t *locals, list_t *args);


static funcdef_t builtin_funcs[] = {
    { "add", TRUE, builtin_add },
    { "sub", TRUE, builtin_sub },
    { "mul", TRUE, builtin_mul },
    { "div", TRUE, builtin_div },
    { "dump-symbols", TRUE, builtin_dump_symbols },
    { "vocab", TRUE, builtin_vocab },
    { "log", FALSE, builtin_log },
    { "say", TRUE, builtin_say },
    { "list", TRUE, builtin_list },
    { "quote", FALSE, builtin_quote },
    { "if", FALSE, builtin_if },
    { "prop-has", TRUE, builtin_prop_has },
    { "prop-get", TRUE, builtin_prop_get },
    { "proc", TRUE, builtin_proc },
    { "parent", TRUE, builtin_parent },
    { "bold", TRUE, builtin_bold },
    { "normal", TRUE, builtin_normal },
    { "reverse", TRUE, builtin_reverse },
    { "prop-true", TRUE, builtin_prop_true },
    { "or", TRUE, builtin_or },
    { "and", TRUE, builtin_and },
    { "not", TRUE, builtin_not },
    { "sibling", TRUE, builtin_sibling },
    { "child", TRUE, builtin_child },
    { "set", TRUE, builtin_set },
    { "object-move", TRUE, builtin_object_move },
    { "contains", TRUE, builtin_contains },
    { "indirectly-contains", TRUE, builtin_contains_indirect },
    { "eq", TRUE, builtin_eq },
    { "is-object", TRUE, builtin_is_object },
    { "is-string", TRUE, builtin_is_string },
    { "is-number", TRUE, builtin_is_number },
    { "is-function", TRUE, builtin_is_function },
    { "is-list", TRUE, builtin_is_list },
    { "type-name", TRUE, builtin_type_name },
    { "prop-set", TRUE, builtin_prop_set },
    { "request-quit", TRUE, builtin_request_quit },
    { "dump-obj", TRUE, builtin_dump_obj },
    { NULL }
};

object_t* list_to_object(list_t *list) {
    if (!list) return NULL;
    if (list->type != T_OBJECT_REF) return NULL;
    return list->ptr;
}

list_t *list_run_function_noargs(gamedata_t *gd, function_t *func) {
    list_t *args = list_create();
    list_t *result = list_run_function(gd, func, args);
    list_free(args);
    return result;
}

list_t *list_run_function(gamedata_t *gd, function_t *func, list_t *args) {
    symboltable_t *locals = symboltable_create();

    list_t *arg_name = func->arg_list->child;
    list_t *cur_arg = args->child;
    while (arg_name) {
        if (cur_arg) {
            symbol_add_ptr(locals, arg_name->text, SYM_LIST, cur_arg);
            cur_arg = cur_arg->next;
        } else {
            symbol_add_value(locals, arg_name->text, SYM_CONSTANT, 0);
        }
        arg_name = arg_name->next;
    }

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
                debug_out("list_evaluate: undefined value %s\n", list->text);
                return list_create_false();
            }
            switch(symbol->type) {
                case SYM_LIST:
                    new_list = list_duplicate(symbol->d.ptr);
                    return new_list;
                case SYM_OBJECT:
                    new_list = list_create();
                    new_list->type = T_OBJECT_REF;
                    new_list->ptr = symbol->d.ptr;
                    return new_list;
                case SYM_FUNCTION:
                    new_list = list_create();
                    new_list->type = T_FUNCTION_REF;
                    new_list->ptr = symbol->d.ptr;
                    return new_list;
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
    if (!list->child) {
        return list_create();
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
        case T_OBJECT_REF:
            debug_out(" [object@%p]", list->ptr);
            break;
        case T_FUNCTION_REF:
            debug_out(" [function@%p]", list->ptr);
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
            case T_OBJECT_REF:
                object_name_print(gd, list->ptr);
                break;
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

list_t* builtin_list(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    return list_duplicate(args);
}

list_t* builtin_quote(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        return list_create();
    } else {
        if (args->child->next) {
            debug_out("builtin_quote: extraneous arguments provided.\n");
        }
        return list_duplicate(args->child);
    }
}

list_t* builtin_if(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        debug_out("builtin_if: no condition supplied.\n");
        return list_create_false();
    }
    list_t *result = list_evaluate(gd, locals, args->child);
    int is_true = list_is_true(result);
    list_free(result);

    list_t *good = args->child->next;
    if (is_true) {
        if (good) {
            result = list_evaluate(gd, locals, good);
        } else {
            result = list_create_true();
        }
    } else {
        if (good && good->next) {
            result = list_evaluate(gd, locals, good->next);
        } else {
            result = list_create_false();
        }
    }
    return result;
}

static list_t* builtin_prop_has(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        return list_create_false();
    }
    object_t *object = args->child->ptr;

    if (!args->child->next || args->child->next->type != T_INTEGER) {
        return list_create_false();
    }
    int prop_id = args->child->next->number;

    property_t *property = object_property_get(object, prop_id);
    if (property) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

static list_t* builtin_prop_get(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        return list_create_false();
    }
    object_t *object = args->child->ptr;

    if (!args->child->next || args->child->next->type != T_INTEGER) {
        return list_create_false();
    }
    int prop_id = args->child->next->number;

    property_t *property = object_property_get(object, prop_id);
    if (!property) {
        return list_create_false();
    }
    list_t *result = NULL;
    switch(property->value.type) {
        case PT_STRING:
            result = list_create();
            result->type = T_STRING;
            result->text = str_dupl(property->value.d.ptr);
            return result;
        case PT_INTEGER:
            result = list_create();
            result->type = T_INTEGER;
            result->number = property->value.d.num;
            return result;
        case PT_OBJECT:
            result = list_create();
            result->type = T_OBJECT_REF;
            result->ptr = property->value.d.ptr;
            return result;
        default:
            debug_out("builtin_prop_get: unknown property type %d\n", property->value.type);
            return list_create_false();
    }
}
/*
#define PT_OBJECT 2
#define PT_ARRAY 3
*/

static list_t* builtin_proc(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    return list_duplicate(args->last);
}

static list_t* builtin_parent(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        debug_out("builtin_parent: called without argument\n");
        return list_create_false();
    }

    if (args->child->type != T_OBJECT_REF) {
        debug_out("builtin_parent: called with non-object\n");
        return list_create_false();
    }

    object_t *object = args->child->ptr;
    list_t *result = list_create();
    result->type = T_OBJECT_REF;
    if (object->parent) {
        result->ptr = object->parent;
    } else {
        result->ptr = gd->root;
    }
    return result;
}

list_t* builtin_bold(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    list_t *result = list_create();
    result->type = T_STRING;
    result->text = str_dupl("\x1b[1m");
    return result;
}

list_t* builtin_normal(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    list_t *result = list_create();
    result->type = T_STRING;
    result->text = str_dupl("\x1b[0m");
    return result;
}

list_t* builtin_reverse(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    list_t *result = list_create();
    result->type = T_STRING;
    result->text = str_dupl("\x1b[7m");
    return result;
}

list_t* builtin_prop_true(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || !args->child->next) {
        debug_out("builtin_prop_true: called with insufficent argument\n");
        return list_create_false();
    }

    if (args->child->type != T_OBJECT_REF) {
        debug_out("builtin_prop_true: called with non-object\n");
        return list_create_false();
    }
    if (args->child->next->type != T_INTEGER) {
        debug_out("builtin_prop_true: called with non-integer\n");
        return list_create_false();
    }

    int raw_result = object_property_is_true(args->child->ptr, args->child->next->number, 0);

    if (raw_result) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_or(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    list_t *iter = args->child;
    while (iter) {
        if (list_is_true(iter)) {
            return list_create_true();
        }
        iter = iter->next;
    }
    return list_create_false();
}
list_t* builtin_and(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    list_t *iter = args->child;
    while (iter) {
        if (!list_is_true(iter)) {
            return list_create_false();
        }
        iter = iter->next;
    }
    return list_create_true();
}
list_t* builtin_not(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        return list_create_false();
    }
    if (list_is_true(args->child)) {
        return list_create_false();
    } else {
        return list_create_true();
    }
}

list_t* builtin_sibling(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        debug_out("builtin_sibling: called without argument\n");
        return list_create_false();
    }

    if (args->child->type != T_OBJECT_REF) {
        debug_out("builtin_sibling: called with non-object\n");
        return list_create_false();
    }

    object_t *object = args->child->ptr;
    list_t *result;
    if (object->sibling) {
        result = list_create();
        result->type = T_OBJECT_REF;
        result->ptr = object->sibling;
    } else {
        result = list_create_false();
    }
    return result;
}

list_t* builtin_child(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        debug_out("builtin_child: called without argument\n");
        return list_create_false();
    }

    if (args->child->type != T_OBJECT_REF) {
        debug_out("builtin_child: called with non-object\n");
        return list_create_false();
    }

    object_t *object = args->child->ptr;
    list_t *result;
    if (object->first_child) {
        result = list_create();
        result->type = T_OBJECT_REF;
        result->ptr = object->first_child;
    } else {
        result = list_create_false();
    }
    return result;
}

list_t* builtin_set(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || !args->child->next) {
        debug_out("builtin_set: called with insufficent arguments\n");
        return list_create_false();
    }

    if (args->child->type != T_STRING) {
        debug_out("builtin_set: called with insufficent arguments\n");
        return list_create_false();
    }
    char *name = args->child->text;

    list_t *value = list_duplicate(args->child->next);
    symbol_add_ptr(locals, name, SYM_LIST, value);
    return list_create_true();
}

list_t* builtin_object_move(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        debug_out("builtin_move: called without argument\n");
        return list_create_false();
    }
    object_t *object = list_to_object(args->child);
    object_t *new_parent;
    if (!args->child->next) {
        new_parent = gd->root;
    } else {
        new_parent = list_to_object(args->child->next);
    }

    if (!object || !new_parent) {
        debug_out("builtin_move: called with bad objects\n");
        return list_create_false();
    }

    object_move(object, new_parent);
    return list_create_true();
}

static list_t* builtin_contains(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        debug_out("builtin_contains: first argument must be object\n");
        return list_create_false();
    }
    if (!args->child->next || args->child->next->type != T_OBJECT_REF) {
        debug_out("builtin_contains: second argument must be object\n");
        return list_create_false();
    }

    object_t *o1 = args->child->ptr;
    object_t *o2 = args->child->next->ptr;

    if (object_contains(o1, o2)) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

static list_t* builtin_contains_indirect(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        debug_out("builtin_contains_indirect: first argument must be object\n");
        return list_create_false();
    }
    if (!args->child->next || args->child->next->type != T_OBJECT_REF) {
        debug_out("builtin_contains_indirect: second argument must be object\n");
        return list_create_false();
    }

    object_t *o1 = args->child->ptr;
    object_t *o2 = args->child->next->ptr;

    if (object_contains_indirect(o1, o2)) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

static list_t* builtin_eq(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || !args->child->next) {
        debug_out("builtin_eq: insufficent arguments\n");
        return list_create_false();
    }

    if (args->child->type != args->child->next->type) {
        return list_create_false();
    }

    switch(args->child->type) {
        case T_STRING:
            return list_create_bool(strcmp(args->child->text, args->child->next->text) == 0);
        case T_INTEGER:
            return list_create_bool(args->child->number == args->child->next->number);
        case T_OBJECT_REF:
        case T_FUNCTION_REF:
            return list_create_bool(args->child->ptr == args->child->next->ptr);
        default:
            debug_out("builtin_eq: unhandled list type %d\n", args->child->type);
            return list_create_false();
    }
}

list_t* builtin_is_object(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (args->child && args->child->type == T_OBJECT_REF) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_is_string(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (args->child && args->child->type == T_STRING) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_is_number(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (args->child && args->child->type == T_INTEGER) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_is_function(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (args->child && args->child->type == T_FUNCTION_REF) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_is_list(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (args->child && args->child->type == T_LIST) {
        return list_create_true();
    } else {
        return list_create_false();
    }
}

list_t* builtin_type_name(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child) {
        return list_create_string("(nothing)");
    }
    switch(args->child->type) {
        case T_STRING:      return list_create_string("string");
        case T_INTEGER:     return list_create_string("number");
        case T_LIST:        return list_create_string("list");
        case T_OBJECT_REF:  return list_create_string("object");
        case T_FUNCTION_REF:return list_create_string("function");
        default:
            debug_out("builtin_type_name: unhandled type %d\n");
            return list_create_string("(unhandled)");
    }
}

list_t* builtin_prop_set(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        debug_out("builtin_prop_set: first argument must be object\n");
        return list_create_false();
    }
    if (!args->child->next || args->child->next->type != T_INTEGER) {
        debug_out("builtin_prop_set: second argument must be property number\n");
        return list_create_false();
    }
    object_t *obj = args->child->ptr;
    int pid = args->child->next->number;
    list_t *new_value = args->child->next->next;

    if (!new_value) {
        object_property_delete(obj, pid);
        return list_create_true();
    }
    switch(new_value->type) {
        case T_STRING:
            object_property_add_string(obj, pid, str_dupl(new_value->text));
            return list_create_true();
        case T_INTEGER:
            object_property_add_integer(obj, pid, new_value->number);
            return list_create_true();
        case T_OBJECT_REF:
            object_property_add_object(obj, pid, new_value->ptr);
            return list_create_true();
        default:
            debug_out("builtin_prop_set: unhandled list type %d\n", new_value->type);
            return list_create_false();
    }
}

static list_t* builtin_request_quit(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    gd->quit_game = TRUE;
    return list_create_true();
}

static list_t* builtin_dump_obj(gamedata_t *gd, symboltable_t *locals, list_t *args) {
    if (!args->child || args->child->type != T_OBJECT_REF) {
        debug_out("builtin_dump_obj: first argument must be object\n");
        return list_create_false();
    }
    object_dump(gd, args->child->ptr);
    return list_create_true();
}
