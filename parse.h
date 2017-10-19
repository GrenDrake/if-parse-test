#ifndef PARSE_H
#define PARSE_H

#define MAX_INPUT_LENGTH 256
#define MAX_INPUT_WORDS 32

#define PI_NAME 1
#define PI_DESC 2
#define PI_NORTH 3
#define PI_EAST 4
#define PI_SOUTH 5
#define PI_WEST 6
#define PI_VOCAB 7
#define PI_IDENT 8
#define PI_INITIAL_PARENT 9

#define PT_INTEGER 0
#define PT_STRING 1
#define PT_OBJECT 2
#define PT_ARRAY 3

#define ACT_QUIT  0
#define ACT_TAKE  1
#define ACT_DROP  2
#define ACT_MOVE  3
#define ACT_INVENTORY 4
#define ACT_LOOK 5
#define ACT_PUTIN 6

#define GT_MAX_TOKENS 16

#define GT_END  0
#define GT_WORD 1
#define GT_ANY  2
#define GT_NOUN 3

#define GF_ALT  1

#define PARSE_MAX_OBJS 64
#define PARSE_MAX_NOUNS 2


typedef struct CMD_TOKEN {
    int word_no;
    const char *word;
    int start;
    int end;
} cmd_token_t;

typedef struct GRAMMAR {
    int type;
    int value;
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
    struct OBJECT *parent;
    struct OBJECT *first_child;
    struct OBJECT *sibling;
} object_t;

typedef struct GAMEDATA {
    const char **dictionary;
    action_t *actions;
    object_t *root;
    object_t *player;

    int quit_game;
    char input[MAX_INPUT_LENGTH];
    cmd_token_t words[MAX_INPUT_WORDS];
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


int vocab_index(const char *word);
int action_add(gamedata_t *gd, action_t *action);


gamedata_t *gamedata_create();
gamedata_t* load_data();
void free_data(gamedata_t *gd);
object_t *object_get_by_ident(gamedata_t *gd, const char *ident);

extern const char *vocab[];

#endif // PARSE_H