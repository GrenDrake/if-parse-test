#include <stdlib.h>
#include <string.h>

#include "parse.h"

static void symbol_add_core(gamedata_t *gd, symbol_t *symbol);

gamedata_t *gamedata_create() {
    gamedata_t *gd = calloc(sizeof(gamedata_t), 1);
    gd->root = object_create(NULL);
    gd->symbols = calloc(sizeof(symboltable_t), 1);
    return gd;
}

unsigned hash_string(const char *text) {
    unsigned hash = 0x811c9dc5;
    size_t len = strlen(text);
    for (size_t i = 0; i < len; ++i) {
        hash ^= text[i];
        hash *= 16777619;
    }
    return hash;
}

gamedata_t* load_data() {
    gamedata_t *gd = parse_file("game.dat");
    if (!gd) {
        printf("Error loading game data.\n");
        return NULL;
    }
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
    symbol_t *symbol = symbol_get(gd, SYM_OBJECT, ident);
    if (symbol) {
        return symbol->d.ptr;
    }
    return NULL;
}

int property_number(gamedata_t *gd, const char *name) {
    static int next_id = 1;
    symbol_t *symbol = symbol_get(gd, SYM_PROPERTY, name);
    if (!symbol) {
        symbol = calloc(sizeof(symbol_t), 1);
        symbol->name = str_dupl(name);
        symbol->type = SYM_PROPERTY;
        symbol->d.value = next_id++;
        symbol_add_core(gd, symbol);
    }

    return symbol->d.value;
}

char *str_dupl(const char *text) {
    size_t length = strlen(text);
    char *new_text = malloc(length + 1);
    strcpy(new_text, text);
    return new_text;
}

char *str_dupl_left(const char *text, int size) {
    char *new_text = malloc(size + 1);
    strncpy(new_text, text, size);
    new_text[size] = 0;
    return new_text;
}

void symbol_add_core(gamedata_t *gd, symbol_t *symbol) {
    unsigned hashcode = hash_string(symbol->name) % SYMBOL_TABLE_BUCKETS;
    if (gd->symbols->buckets[hashcode] != NULL) {
        symbol->next = gd->symbols->buckets[hashcode];
    }
    gd->symbols->buckets[hashcode] = symbol;
}

void symbol_add_ptr(gamedata_t *gd, const char *name, int type, void *value) {
    symbol_t *symbol = calloc(sizeof(gamedata_t), 1);
    symbol->name = str_dupl(name);
    symbol->type = type;
    symbol->d.ptr = value;

    symbol_add_core(gd, symbol);
}

void symbol_add_value(gamedata_t *gd, const char *name, int type, int value) {
    symbol_t *symbol = calloc(sizeof(gamedata_t), 1);
    symbol->name = str_dupl(name);
    symbol->type = type;
    symbol->d.value = value;

    symbol_add_core(gd, symbol);
}

symbol_t* symbol_get(gamedata_t *gd, int type, const char *name) {
    unsigned hashcode = hash_string(name) % SYMBOL_TABLE_BUCKETS;

    if (gd->symbols->buckets[hashcode] == NULL) {
        return NULL;
    }

    symbol_t *symbol = gd->symbols->buckets[hashcode];
    while (symbol) {
        if (symbol->type == type && strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}
