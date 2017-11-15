#ifndef PARSE_H
#define PARSE_H

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_BUILD 0

#define MAX_INPUT_LENGTH 256
#define MAX_INPUT_WORDS 32

#define PARSE_AMBIG -1
#define PARSE_BADNOUN -2
#define PARSE_NONMATCH -3
#define PARSE_BADTOKEN -4
#define OBJ_AMBIG ((object_t*)-1)

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
#define ACT_DUMPOBJ 8

#define GT_MAX_TOKENS 16

#define GT_END  0
#define GT_WORD 1
#define GT_ANY  2
#define GT_NOUN 3
#define GT_SCOPE 4

#define GF_ALT  1

#define SYM_OBJECT 0
#define SYM_PROPERTY 1
#define SYM_CONSTANT 2
#define SYM_FUNCTION 3

#define PARSE_MAX_OBJS 64
#define PARSE_MAX_NOUNS 2
#define SYMBOL_TABLE_BUCKETS 32

#define T_LIST    0
#define T_ATOM    1
#define T_STRING  2
#define T_INTEGER 3
#define T_VOCAB   4

#define T_OPEN    98
#define T_CLOSE   99

typedef struct TOKEN {
    int type;
    int number;
    char *text;

    struct TOKEN *prev;
    struct TOKEN *next;
} token_t;

typedef struct LIST {
    int type;
    int number;
    char *text;
    struct LIST *child;
    struct LIST *last;

    struct LIST *next;
} list_t;

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

typedef struct FUNCTION {
    const char *name;
    symboltable_t symbols;
    list_t *body;
} function_t;

typedef struct GRAMMAR {
    int type;
    int value;
    void *ptr;
    int flags;
} grammar_t;

typedef struct ACTION {
    int action_code;
    const char *action_name;
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

typedef struct NOUN {
    object_t *object;
    struct NOUN *ambig;
    struct NOUN *also;
} noun_t;

typedef struct CMD_TOKEN {
    int word_no;
    const char *word;
} cmd_token_t;

typedef struct COMMAND_INPUT {
    char *input;
    int next_cmd;

    unsigned word_count, cur_word;
    cmd_token_t words[MAX_INPUT_WORDS];

    int action;
    unsigned noun_count;
    noun_t *nouns[PARSE_MAX_NOUNS];
} input_t;

typedef struct GAMEDATA {
    const char **dictionary;
    action_t *actions;
    object_t *root;
    object_t *gameinfo;
    object_t *player;
    symboltable_t *symbols;

    int quit_game;
    int search_count;
    object_t *search[PARSE_MAX_OBJS];
} gamedata_t;


extern const char *symbol_types[];


void dump_list(FILE *dest, list_t *list);
void dump_tokens(FILE *dest, token_t *tokens);

void token_free(token_t *token);

void list_add(list_t *list, list_t *item);
list_t *list_create();
list_t *list_create_false();
list_t *list_create_true();
list_t *list_duplicate(list_t *old_list);
void list_free(list_t *list);
int list_size(list_t *list);


int object_contains(object_t *container, object_t *content);
int object_contains_indirect(object_t *container, object_t *content);
object_t* object_create(object_t *parent);
void object_dump(gamedata_t *gd, object_t *obj);
void object_free(object_t *obj);
void object_move(object_t *obj, object_t *new_parent);
void object_property_add_array(object_t *obj, int pid, int size);
void object_property_add_core(object_t *obj, property_t *prop); /* non-public */
void object_property_add_integer(object_t *obj, int pid, int value);
void object_property_add_object(object_t *obj, int pid, object_t *value);
void object_property_add_string(object_t *obj, int pid, const char *text);
void object_property_delete(object_t *obj, int pid);
property_t* object_property_get(object_t *obj, int pid);
int object_property_is_true(object_t *obj, int pid, int default_value);

void objectloop_depth_first(object_t *root, void (*callback)(object_t *obj));
void objectloop_free(object_t *obj);

void property_free(property_t *prop);


void vocab_dump();
void vocab_raw_add(const char *the_word);
void vocab_build();
void vocab_raw_free();
int vocab_index(const char *word);
int action_add(gamedata_t *gd, action_t *action);


gamedata_t *gamedata_create();
unsigned hash_string(const char *text);
symboltable_t* symboltable_create();
gamedata_t* load_data();
void free_data(gamedata_t *gd);
object_t *object_get_by_ident(gamedata_t *gd, const char *ident);
int property_number(gamedata_t *gd, const char *name);
char *str_dupl(const char *text);
char *str_dupl_left(const char *text, int size);
void symbol_add_value(symboltable_t *table, const char *name, int type, int value);
void symbol_add_ptr(symboltable_t *table, const char *name, int type, void *value);
symbol_t* symbol_get(symboltable_t *table, const char *name);
void symboltable_free(symboltable_t *table);


void dump_symbol_table(FILE *fp, gamedata_t *gd);
int tokenize_file(const char *filename);
gamedata_t* parse_tokens();
list_t* parse_string(const char *text);

void debug_out(const char *msg, ...);
void text_out(const char *msg, ...);
char* read_line();
void style_bold();
void style_normal();
void style_reverse();


void object_name_print(gamedata_t *gd, object_t *obj);
void object_property_print(object_t *obj, int prop_num);
void print_list_horz(gamedata_t *gd, object_t *parent_obj);
void print_list_vert_core(gamedata_t *gd, object_t *parent_obj, int depth);
void print_list_vert(gamedata_t *gd, object_t *parent_obj);
void print_location(gamedata_t *gd, object_t *location);

list_t *list_run(gamedata_t *gd, symboltable_t *locals, list_t *list);

#endif // PARSE_H
