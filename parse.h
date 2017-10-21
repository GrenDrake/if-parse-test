#ifndef PARSE_H
#define PARSE_H

#define MAX_INPUT_LENGTH 256
#define MAX_INPUT_WORDS 32

#define PARSE_AMBIG -1
#define PARSE_BADNOUN -2
#define PARSE_NONMATCH -3
#define PARSE_BADTOKEN -4

#define PI_NAME 1
#define PI_DESC 2
#define PI_NORTH 3
#define PI_EAST 4
#define PI_SOUTH 5
#define PI_WEST 6
#define PI_VOCAB 7
#define PI_IDENT 8

#define PT_STRING 1
#define PT_OBJECT 2
#define PT_ARRAY 3
#define PT_INTEGER 4
#define PT_TMPNAME 99

#define ACT_QUIT  0
#define ACT_TAKE  1
#define ACT_DROP  2
#define ACT_MOVE  3
#define ACT_INVENTORY 4
#define ACT_LOOK 5
#define ACT_PUTIN 6
#define ACT_EXAMINE 7

#define GT_MAX_TOKENS 16

#define GT_END  0
#define GT_WORD 1
#define GT_ANY  2
#define GT_NOUN 3
#define GT_SCOPE 4

#define GF_ALT  1

#define SYM_OBJECT 0
#define SYM_PROPERTY 1

#define PARSE_MAX_OBJS 64
#define PARSE_MAX_NOUNS 2
#define SYMBOL_TABLE_BUCKETS 32

typedef struct SYMBOL_INFO {
    char *name;
    int type;
    union {
        void *ptr;
        int value;
    } d;

    struct SYMBOL_INFO *next;
} symbol_t;

typedef struct SYMBOL_TABLE {
    symbol_t *buckets[SYMBOL_TABLE_BUCKETS];
} symboltable_t;

typedef struct CMD_TOKEN {
    int word_no;
    const char *word;
    int start;
    int end;
} cmd_token_t;

typedef struct GRAMMAR {
    int type;
    int value;
    void *ptr;
    int flags;
} grammar_t;

typedef struct ACTION {
    int action_code;
    grammar_t grammar[GT_MAX_TOKENS];

    struct ACTION *next;
} action_t;

typedef struct VALUE {
    int type;
    int array_size;
    union {
        int num;
        void *ptr;
    } d;
} value_t;
typedef struct PROPERTY {
    int id;
    value_t value;

    struct PROPERTY *next;
} property_t;

typedef struct OBJECT {
    int id;
    property_t *properties;

    const char *parent_name;

    struct OBJECT *parent;
    struct OBJECT *first_child;
    struct OBJECT *sibling;
} object_t;

typedef struct GAMEDATA {
    const char **dictionary;
    action_t *actions;
    object_t *root;
    object_t *player;
    symboltable_t *symbols;

    int quit_game;
    char input[MAX_INPUT_LENGTH];
    cmd_token_t words[MAX_INPUT_WORDS];
    int cur_word;
    int action;
    int search_count;
    object_t *search[PARSE_MAX_OBJS];
    int noun_count;
    object_t *objects[PARSE_MAX_NOUNS];
} gamedata_t;


int object_contains(object_t *container, object_t *content);
int object_contains_indirect(object_t *container, object_t *content);
object_t* object_create(object_t *parent);
void object_free(object_t *obj);
void object_free_property(property_t *prop);
void object_move(object_t *obj, object_t *new_parent);
void object_property_add_array(object_t *obj, int pid, int size);
void object_property_add_core(object_t *obj, property_t *prop); /* non-public */
void object_property_add_integer(object_t *obj, int pid, int value);
void object_property_add_object(object_t *obj, int pid, object_t *value);
void object_property_add_string(object_t *obj, int pid, const char *text);
property_t* object_property_get(object_t *obj, int pid);

void objectloop_depth_first(object_t *root, void (*callback)(object_t *obj));
void objectloop_free(object_t *obj);


void vocab_dump();
void vocab_raw_add(const char *the_word);
void vocab_build();
void vocab_raw_free();
int vocab_index(const char *word);
int action_add(gamedata_t *gd, action_t *action);


gamedata_t *gamedata_create();
unsigned hash_string(const char *text);
gamedata_t* load_data();
void free_data(gamedata_t *gd);
object_t *object_get_by_ident(gamedata_t *gd, const char *ident);
int property_number(gamedata_t *gd, const char *name);
char *str_dupl(const char *text);
char *str_dupl_left(const char *text, int size);
void symbol_add_value(gamedata_t *gd, const char *name, int type, int value);
void symbol_add_ptr(gamedata_t *gd, const char *name, int type, void *value);
symbol_t* symbol_get(gamedata_t *gd, int type, const char *name);


gamedata_t* parse_file(const char *filename);

#endif // PARSE_H
