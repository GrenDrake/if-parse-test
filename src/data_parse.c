#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

// tokenizing
static int valid_identifier(int ch);
static token_t *tokenize(char *file, int allow_new_vocab);

// parsing
static int parse_action(gamedata_t *gd, list_t *list);
static list_t* parse_list(token_t **place);
static int parse_object(gamedata_t *gd, list_t *list);

static list_t* parse_file(const char *filename);
static list_t *parse_tokens_to_lists(token_t *tokens);
static int parse_lists_toplevel(gamedata_t *gd, list_t *lists);
static int fix_references(gamedata_t *gd);


/* ****************************************************************************
 * Dumping data to a stream (for debugging)
 * ****************************************************************************/
void dump_symbol_table(FILE *fp, gamedata_t *gd) {
    fprintf(fp, "======================================================\n");
    fprintf(fp, "Symbol Name                       Type      Value\n");
    fprintf(fp, "------------------------------------------------------\n");
    for (int i = 0; i < SYMBOL_TABLE_BUCKETS; ++i) {
        symbol_t *symbol = gd->symbols->buckets[i];
        while (symbol) {
            fprintf(fp, "%-32s  %-8s  %p\n", symbol->name, symbol_types[symbol->type], symbol->d.ptr);
            symbol = symbol->next;
        }
    }
    fprintf(fp, "======================================================\n");
}


/* ****************************************************************************
 * Tokenizing
 * ****************************************************************************/
int valid_identifier(int ch) {
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '#') {
        return 1;
    }
    return 0;
}

void token_add(token_t **tokens, token_t **last_ptr, token_t *token) {
    if (*last_ptr == NULL) {
        *tokens = *last_ptr = token;
    }
    else {
        (*last_ptr)->next = token;
        *last_ptr = token;
    }
}

token_t *tokenize(char *file, int allow_new_vocab) {
    if (!file) return NULL;

    token_t *tokens = NULL, *last_ptr = NULL;
    size_t pos = 0, filesize = strlen(file);
    while (pos < filesize) {
        if (file[pos] == '/' && pos+1 < filesize && file[pos+1] == '/') {
            while (pos < filesize && file[pos] != '\n') {
                ++pos;
            }
        } else if (isspace(file[pos])) {
            ++pos;
        } else if (file[pos] == '(') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_OPEN;
            token_add(&tokens, &last_ptr, t);
            ++pos;
        } else if (file[pos] == ')') {
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_CLOSE;
            token_add(&tokens, &last_ptr, t);
            ++pos;
        } else if (isdigit(file[pos])) {
            int start = pos;
            char *token = &file[pos];
            while (isdigit(file[pos])) {
                ++pos;
            }
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_INTEGER;
            char *tmp = str_dupl_left(token, pos - start);
            t->number = strtol(tmp, 0, 0);
            free(tmp);
            token_add(&tokens, &last_ptr, t);
        } else if (valid_identifier(file[pos])) {
            int start = pos;
            char *token = &file[pos];
            while (valid_identifier(file[pos])) {
                ++pos;
            }
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_ATOM;
            t->text = str_dupl_left(token, pos - start);
            token_add(&tokens, &last_ptr, t);
        } else if (file[pos] == '"') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '"') {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_STRING;
            t->text = str_dupl(token);
            token_add(&tokens, &last_ptr, t);
        } else if (file[pos] == '<') {
            ++pos;
            char *token = &file[pos];
            while (pos < filesize && file[pos] != '>') {
                ++pos;
            }
            file[pos++] = 0;
            token_t *t = calloc(sizeof(token_t), 1);
            t->type = T_VOCAB;
            if (allow_new_vocab) {
                vocab_raw_add(token);
                t->text = str_dupl(token);
            } else {
                t->number = vocab_index(token);
            }
            token_add(&tokens, &last_ptr, t);
        } else {
            text_out("Unexpected token '%c' (%d).\n", file[pos], file[pos]);
            ++pos;
        }
    }
    return tokens;
}


/* ****************************************************************************
 * Parse functions
 * ****************************************************************************/
int parse_action(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"action") || list->child->next == NULL) {
        return 0;
    }

    list_t *cur = list->child->next;
    action_t *act = calloc(sizeof(action_t), 1);
    if (cur->type == T_INTEGER) {
        act->action_code = cur->number;
        act->action_name = NULL;
    } else if (cur->type == T_ATOM) {
        act->action_code = 0;
        act->action_name = str_dupl(cur->text);
    } else {
        text_out("Action number must be integer or atom.\n");
        return 0;
    }

    cur = cur->next;
    if (!cur) {
        text_out("Action has no grammar.\n");
        return 0;
    }

    int pos = 0;
    list_t *sub;
    while (cur) {
        switch(cur->type) {
            case T_VOCAB:
                act->grammar[pos].type = GT_WORD;
                act->grammar[pos].ptr = str_dupl(cur->text);
                ++pos;
                break;
            case T_ATOM:
                if (strcmp(cur->text, "noun") == 0) {
                    act->grammar[pos].type = GT_NOUN;
                    ++pos;
                } else if (strcmp(cur->text, "any") == 0) {
                    act->grammar[pos].type = GT_ANY;
                    ++pos;
                } else if (strcmp(cur->text, "scope") == 0) {
                    act->grammar[pos].type = GT_SCOPE;
                    cur = cur->next;
                    if (!cur || cur->type != T_ATOM) {
                        text_out("scope grammar token must be followed by object name.\n");
                        return 0;
                    }
                    act->grammar[pos].ptr = str_dupl(cur->text);
                    ++pos;
                } else {
                    text_out("Unrecognized grammar token '%s'.\n", cur->text);
                    return 0;
                }
                break;
            case T_LIST:
                sub = cur->child;
                while (sub) {
                    switch (sub->type) {
                        case T_VOCAB:
                            act->grammar[pos].type = GT_WORD;
                            act->grammar[pos].ptr = str_dupl(sub->text);
                            if (sub->next) {
                                act->grammar[pos].flags |= GF_ALT;
                            }
                            ++pos;
                            break;
                        default:
                            text_out("Only vocab values permitted in action sub-statement. (%d)\n", sub->type);
                            return 0;
                    }
                    sub = sub->next;
                }
                break;
            case T_STRING:
            case T_INTEGER:
                text_out("Integer and string values not permitted in action statement.\n");
                return 0;
            default:
                text_out("Bad token type %d in action definition.\n", cur->type);
        }
        cur = cur->next;
    }
    action_add(gd, act);

    return 1;
}

int parse_constant(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"constant") || list->child->next == NULL) {
        return 0;
    }

    list_t *cur = list->child->next;
    if (cur->type != T_ATOM) {
        text_out("Constant name must be atom.\n");
        return 0;
    }
    char *name = cur->text;
    cur = cur->next;
    if (!cur) {
        text_out("Constant has no value.\n");
        return 0;
    }
    if (cur->next) {
        text_out("Constant may have only one value.\n");
        return 0;
    }
    switch(cur->type) {
        case T_INTEGER:
            symbol_add_value(gd->symbols, name, SYM_CONSTANT, cur->number);
            break;
        default:
            text_out("Constant has unsopported value tyoe.\n");
    }

    return 1;
}

list_t* parse_list(token_t **place) {
    token_t *cur = *place;

    if (cur->type != T_OPEN) {
        text_out("Expected '('\n");
        return NULL;
    }
    cur = cur->next;

    list_t *list = list_create();
    while (cur && cur->type != T_CLOSE) {
        if (cur->type == T_OPEN) {
            list_t *sublist = parse_list(&cur);
            list_add(list, sublist);
        } else {
            if (cur->type == T_INTEGER) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_INTEGER;
                item->number = cur->number;
                list_add(list, item);
            } else if (cur->type == T_ATOM) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_ATOM;
                item->text = str_dupl(cur->text);
                list_add(list, item);
            } else if (cur->type == T_STRING) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_STRING;
                item->text = str_dupl(cur->text);
                list_add(list, item);
            } else if (cur->type == T_VOCAB) {
                list_t *item = calloc(sizeof(list_t), 1);
                item->type = T_VOCAB;
                item->text = str_dupl(cur->text);
                list_add(list, item);
            }
            cur = cur->next;
        }

    }
    if (cur == NULL) {
        text_out("Unexpected end of tokens\n");
        *place = cur;
        return NULL;
    }
    cur = cur->next;

    *place = cur;
    return list;
}

int parse_function(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"function") || list->child->next == NULL) {
        return 0;
    }

    list_t *cur = list->child->next;
    function_t *func = calloc(sizeof(function_t), 1);
    if (cur->type != T_ATOM) {
        text_out("Function name must be atom.\n");
        return 0;
    }
    func->name = str_dupl(cur->text);
    symbol_add_ptr(gd->symbols, func->name, SYM_FUNCTION, (void*)func);

    cur = cur->next;
    if (!cur || cur->type != T_LIST) {
        text_out("Function has no argument list. (Use empty list if no arguments.)\n");
        return 0;
    }
    // handle arguments

    cur = cur->next;
    func->body = list_duplicate(cur);
    return 1;
}

#define MAX_PROPERTY_NAME 64
int parse_object(gamedata_t *gd, list_t *list) {
    if (list->type != T_LIST || list->child == NULL || list->child->type != T_ATOM
            || strcmp(list->child->text,"object")) {
        return 0;
    }

    int ident_prop = property_number(gd, "#internal-name");

    object_t *obj = object_create(gd->root);
    list_t *prop = list->child->next;
    list_t *val  = prop->next;
    if (prop->type != T_ATOM || val->type != T_ATOM) {
        text_out("Object name and parent must be atom.\n");
        return 0;
    }
    if (strcmp(prop->text, "-") != 0) {
        object_property_add_string(obj, ident_prop, str_dupl(prop->text));
        symbol_add_ptr(gd->symbols, prop->text, SYM_OBJECT, obj);
    }
    if (strcmp(val->text, "-") != 0) {
        obj->parent_name = str_dupl(val->text);
    }
    prop = val->next;

    if (prop) val = prop->next;
    char full_prop_name[MAX_PROPERTY_NAME] = { '#' };
    while (prop) {
        if (val == NULL) {
            text_out("Found property without value.\n");
            return 0;
        }
        if (prop->type != T_ATOM) {
            text_out("Property name must be atom.\n");
            return 0;
        }

        strncpy(&full_prop_name[1], prop->text, MAX_PROPERTY_NAME-2);
        int p_num = property_number(gd, full_prop_name);
        if (val->type == T_STRING) {
            object_property_add_string(obj, p_num, str_dupl(val->text));
        } else if (val->type == T_ATOM) {
            object_property_add_string(obj, p_num, str_dupl(val->text));
            property_t *p = object_property_get(obj, p_num);
            p->value.type = PT_TMPNAME;
        } else if (val->type == T_VOCAB) {
            object_property_add_string(obj, p_num, str_dupl(val->text));
            property_t *p = object_property_get(obj, p_num);
            p->value.type = PT_TMPVOCAB;
        } else if (val->type == T_INTEGER) {
            object_property_add_integer(obj, p_num, val->number);
        } else if (val->type == T_LIST) {
            object_property_add_array(obj, p_num, list_size(val));
            property_t *p = object_property_get(obj, p_num);
            value_t *arr = p->value.d.ptr;
            list_t *cur = val->child;
            int counter = 0;
            while (cur) {
                switch(cur->type) {
                    case T_INTEGER:
                        arr[counter].type = PT_INTEGER;
                        arr[counter].d.num = cur->number;
                        break;
                    case T_VOCAB:
                        arr[counter].type = PT_TMPVOCAB;
                        arr[counter].d.ptr = str_dupl(cur->text);
                        break;
                    case T_STRING:
                        arr[counter].type = PT_STRING;
                        arr[counter].d.ptr = str_dupl(cur->text);
                        break;
                    case T_ATOM:
                        arr[counter].type = PT_TMPNAME;
                        arr[counter].d.ptr = str_dupl(cur->text);
                        break;
                    case T_LIST:
                        text_out("Nested lists are not permitted in object properties.\n");
                        break;
                    default:
                        text_out("WARNING: unhandled array value type %d.\n", cur->type);
                }
                ++counter;
                cur = cur->next;
            }
        } else {
            text_out("WARNING: unhandled property type %d.\n", val->type);
        }

        prop = val->next;
        if (prop) {
            val = prop->next;
        } else {
            val = NULL;
        }
    }

    return 1;
}


/* ****************************************************************************
 * Parsing source files
 * ****************************************************************************/
token_t *master_token_list = NULL;

gamedata_t* load_data() {
    const char *filelist[] = {
        "game.dat",
        "game2.dat",
        NULL
    };
    gamedata_t *gd = gamedata_create();
    int found_error = FALSE;

    for (int i = 0; filelist[i] != NULL; ++i) {
        list_t *lists = parse_file(filelist[i]);
        if (!lists) {
            debug_out("load_data: failed parse %s (1)\n", filelist[i]);
            found_error = TRUE;
            continue;
        }

        if (!parse_lists_toplevel(gd, lists)) {
            debug_out("load_data: failed parse %s (2)\n", filelist[i]);
            found_error = TRUE;
            continue;
        }

        list_freelist(lists);
    }

    if (found_error) {
        free_data(gd);
        return NULL;
    }

    debug_out("load_data: finalizing loaded data\n");
    vocab_build();
    if (!fix_references(gd)) {
        debug_out("load_data: failed to update references\n");
        free_data(gd);
        return NULL;
    }

    debug_out("load_data: completed loading game data\n");
    return gd;
}

void free_data(gamedata_t *gd) {
    objectloop_free(gd->root);

    while (gd->actions) {
        action_t *next = gd->actions->next;
        free(gd->actions);
        gd->actions = next;
    }

    symboltable_free(gd->symbols);
    free(gd);
}

list_t* parse_string(const char *text) {
    char *work_text = str_dupl(text);
    token_t *tokens = tokenize(work_text, 0);
    list_t *lists = parse_tokens_to_lists(tokens);
    token_freelist(tokens);
    free(work_text);
    return lists;
}

list_t* parse_file(const char *filename) {
    debug_out("parse_file: parsing %s\n", filename);

    char *file = read_file(filename);
    token_t *tokens = tokenize(file, 1);
    list_t *lists = parse_tokens_to_lists(tokens);
    token_freelist(tokens);
    free(file);

    debug_out("parse_file: completed %s\n", filename);
    return lists;
}

list_t *parse_tokens_to_lists(token_t *tokens) {
    list_t *lists = NULL, *last_list = NULL;
    token_t *list_pos = tokens;
    while (list_pos) {
        list_t *list = parse_list(&list_pos);
        if (!list) {
            return NULL;
        }
        if (lists) {
            last_list->next = list;
            last_list = list;
        } else {
            lists = list;
            last_list = list;
        }
    }
    return lists;
}

int parse_lists_toplevel(gamedata_t *gd, list_t *lists) {
    list_t *clist = lists;
    while (clist) {
        if (clist->type != T_LIST) {
            debug_out("parse_lists_toplevel: expected list at top level.\n");
            return 0;
        }
        if (clist->child == NULL) {
            debug_out("parse_lists_toplevel: empty list found at top level.\n");
            return 0;
        }
        if (clist->child->type != T_ATOM) {
            debug_out("parse_lists_toplevel: list must start with atom.\n");
            return 0;
        }

        if (strcmp(clist->child->text, "object") == 0) {
            if (!parse_object(gd, clist)) {
                return 0;
            }
        } else if (strcmp(clist->child->text, "action") == 0) {
            if (!parse_action(gd, clist)) {
                return 0;
            }
        } else if (strcmp(clist->child->text, "constant") == 0) {
            if (!parse_constant(gd, clist)) {
                return 0;
            }
        } else if (strcmp(clist->child->text, "function") == 0) {
            if (!parse_function(gd, clist)) {
                return 0;
            }
        } else {
            debug_out("parse_lists_toplevel: unknown top level construct %s.\n", clist->child->text);
            return 0;
        }

        clist = clist->next;
    }

    return 1;
}

int fix_references(gamedata_t *gd) {
    object_t *curo = gd->root->first_child;
    while (curo) {
        object_t *next = curo->sibling;
        if (curo->parent_name) {
            object_t *parent = object_get_by_ident(gd, curo->parent_name);
            if (!parent) {
                debug_out("fix_references: unknown object name %s.\n", curo->parent_name);
                return 0;
            }
            object_move(curo, parent);
            free((void*)curo->parent_name);
            curo->parent_name = NULL;
        }

        property_t *p = curo->properties;
        while (p) {
            property_t *next = p->next;
            if (p->value.type == PT_TMPNAME) {
                object_t *obj = object_get_by_ident(gd, p->value.d.ptr);
                if (!obj) {
                    text_out("fix_references: undefined reference to %s.\n", (char*)p->value.d.ptr);
                    return 0;
                }
                object_property_add_object(curo, p->id, obj);
            } else if (p->value.type == PT_TMPVOCAB) {
                int vocab_num = vocab_index(p->value.d.ptr);
                free(p->value.d.ptr);
                object_property_add_integer(curo, p->id, vocab_num);
            } else if (p->value.type == PT_ARRAY) {
                for (int i = 0; i < p->value.array_size; ++i) {
                    value_t *val = &((value_t*)p->value.d.ptr)[i];
                    if (val->type == PT_TMPVOCAB) {
                        int vocab_num = vocab_index(val->d.ptr);
                        free(val->d.ptr);
                        val->d.num = vocab_num;
                        val->type = PT_INTEGER;
                    }
                }
            }
            p = next;
        }

        curo = next;
    }

    action_t *cura = gd->actions;
    while (cura) {
        if (cura->action_name) {
            symbol_t *symbol = symbol_get(gd->symbols, cura->action_name);
            if (!symbol) {
                text_out("Action code contains unknown symbol %s.\n", cura->action_name);
            }
            cura->action_code = symbol->d.value;
            free((void*)cura->action_name);
            cura->action_name = NULL;
        }
        for (int i = 0; i < GT_MAX_TOKENS; ++i) {
            if (cura->grammar[i].type == GT_SCOPE) {
                char *name = cura->grammar[i].ptr;
                cura->grammar[i].ptr = object_get_by_ident(gd, name);
                if (!cura->grammar[i].ptr) {
                    text_out("Action scope contains unknown object %s.\n", name);
                    return 0;
                }
                free(name);
            } else if (cura->grammar[i].type == GT_WORD) {
                cura->grammar[i].value = vocab_index(cura->grammar[i].ptr);
                free(cura->grammar[i].ptr);
            }
        }
        cura = cura->next;
    }
    return 1;
}
