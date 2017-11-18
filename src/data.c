#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

static void symbol_add_core(symboltable_t *table, symbol_t *symbol);

gamedata_t *gamedata_create() {
    gamedata_t *gd = calloc(sizeof(gamedata_t), 1);
    gd->root = object_create(NULL);
    gd->symbols = symboltable_create();
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

symboltable_t* symboltable_create() {
    symboltable_t *table = calloc(sizeof(symboltable_t), 1);
    return table;
}

void symboltable_free(symboltable_t *table) {
    for (int i = 0; i < SYMBOL_TABLE_BUCKETS; ++i) {
        symbol_t *cur = table->buckets[i], *next;
        while (cur) {
            next = cur->next;
            free(cur->name);
            free(cur);
            cur = next;
        }
    }
    free(table);
}

object_t *object_get_by_ident(gamedata_t *gd, const char *ident) {
    symbol_t *symbol = symbol_get(gd->symbols, ident);
    if (symbol && symbol->type == SYM_OBJECT) {
        return symbol->d.ptr;
    }
    return NULL;
}

int property_number(gamedata_t *gd, const char *name) {
    static int next_id = 1;
    symbol_t *symbol = symbol_get(gd->symbols, name);
    if (!symbol && !gd->game_loaded) {
        symbol = calloc(sizeof(symbol_t), 1);
        symbol->name = str_dupl(name);
        symbol->type = SYM_PROPERTY;
        symbol->d.value = next_id++;
        symbol_add_core(gd->symbols, symbol);
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

void symbol_add_core(symboltable_t *table, symbol_t *symbol) {
    unsigned hashcode = hash_string(symbol->name) % SYMBOL_TABLE_BUCKETS;
    if (table->buckets[hashcode] != NULL) {
        symbol->next = table->buckets[hashcode];
    }
    table->buckets[hashcode] = symbol;
}

void symbol_add_ptr(symboltable_t *table, const char *name, int type, void *value) {
    symbol_t *symbol = calloc(sizeof(gamedata_t), 1);
    symbol->name = str_dupl(name);
    symbol->type = type;
    symbol->d.ptr = value;

    symbol_add_core(table, symbol);
}

void symbol_add_value(symboltable_t *table, const char *name, int type, int value) {
    symbol_t *symbol = calloc(sizeof(gamedata_t), 1);
    symbol->name = str_dupl(name);
    symbol->type = type;
    symbol->d.value = value;

    symbol_add_core(table, symbol);
}

symbol_t* symbol_get(symboltable_t *table, const char *name) {
    unsigned hashcode = hash_string(name) % SYMBOL_TABLE_BUCKETS;

    if (table->buckets[hashcode] == NULL) {
        return NULL;
    }

    symbol_t *symbol = table->buckets[hashcode];
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }

    return NULL;
}
