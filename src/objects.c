#include <stdlib.h>

#include "parse.h"

int object_contains(object_t *container, object_t *content) {
    if (!container || !content) return 0;
    object_t *cur = container->first_child;
    while (cur) {
        if (cur == content) {
            return 1;
        }
        cur = cur->sibling;
    }
    return 0;
}

int object_contains_indirect(object_t *container, object_t *content) {
    if (!container || !content) return 0;
    object_t *cur = container->first_child;
    while (cur) {
        if (cur == content) {
            return 1;
        }
        if (object_contains_indirect(cur, content)) {
            return 1;
        }
        cur = cur->sibling;
    }
    return 0;
}

object_t* object_create(object_t *parent) {
    static int next_id = 0;
    object_t *obj = calloc(sizeof(object_t), 1);
    obj->id = next_id++;
    if (parent) {
        obj->parent = parent;
        obj->sibling = parent->first_child;
        parent->first_child = obj;
    }
    return obj;
}

void object_dump(gamedata_t *gd, object_t *obj) {
    if (!obj) return;

    printf("Object \"");
    object_name_print(gd, obj);
    printf("\" at %p\n", (void*)obj);
    printf("   PARENT: ");

    if (obj->parent) {
        putchar('"');
        object_name_print(gd, obj->parent);
        printf("\" at %p\n", (void*)obj->parent);
    } else {
        printf("(none)\n");
    }

    printf("   FIRST CHILD: ");
    if (obj->first_child) {
        putchar('"');
        object_name_print(gd, obj->first_child);
        printf("\" at %p\n", (void*)obj->first_child);
    } else {
        printf("(none)\n");
    }

    printf("   SIBLING: ");
    if (obj->sibling) {
        putchar('"');
        object_name_print(gd, obj->sibling);
        printf("\" at %p\n", (void*)obj->sibling);
    } else {
        printf("(none)\n");
    }

    if (!obj->properties) {
        printf("   (no properties)\n");
        return;
    }

    property_t *prop = obj->properties;
    while (prop) {
        printf("   %2d ", prop->id);
        switch(prop->value.type) {
            case PT_INTEGER:
                printf("%d\n", prop->value.d.num);
                break;
            case PT_STRING:
                printf("\"%s\"\n", (char*)prop->value.d.ptr);
                break;
            case PT_OBJECT:
                putchar('"');
                object_name_print(gd, (object_t*)prop->value.d.ptr);
                printf("\" at %p\n", (void*)prop->value.d.ptr);
                break;
            case PT_ARRAY:
                putchar('(');
                for (int i = 0; i < prop->value.array_size; ++i) {
                    value_t *cur = &((value_t*)prop->value.d.ptr)[i];
                    printf(" %d", cur->d.num);
                }
                printf(" )\n");
                break;
            default:
                printf("(unhandled type %d)\n", prop->value.type);
                break;
        }
        prop = prop->next;
    }
}

void object_free(object_t *obj) {
    property_t *cur = obj->properties;
    property_t *next = NULL;
    while (cur) {
        next = cur->next;
        property_free(cur);
        cur = next;
    }
    free(obj);
}

void object_move(object_t *obj, object_t *new_parent) {
    if (!obj || !new_parent || obj == new_parent || obj->parent == new_parent || !obj->parent) {
        return;
    }

    // remove from object tree
    if (obj->parent->first_child == obj) {
        obj->parent->first_child = obj->sibling;
    } else {
        object_t *cur = obj->parent->first_child;
        while (cur && cur->sibling != obj) {
            cur = cur->sibling;
        }
        if (!cur) return;
        cur->sibling = obj->sibling;
    }
    obj->sibling = new_parent->first_child;
    obj->parent = new_parent;
    new_parent->first_child = obj;
}


void object_property_add_array(object_t *obj, int pid, int size) {
    property_t *prop = calloc(sizeof(property_t), 1);
    prop->id = pid;
    prop->value.type = PT_ARRAY;
    prop->value.d.ptr = calloc(sizeof(value_t), size);
    prop->value.array_size = size;

    object_property_add_core(obj, prop);
}

void object_property_add_core(object_t *obj, property_t *prop) {
    object_property_delete(obj, prop->id);
    prop->next = obj->properties;
    obj->properties = prop;
}

void object_property_add_integer(object_t *obj, int pid, int value) {
    property_t *prop = calloc(sizeof(property_t), 1);
    prop->id = pid;
    prop->value.type = PT_INTEGER;
    prop->value.d.num = value;

    object_property_add_core(obj, prop);
}

void object_property_add_object(object_t *obj, int pid, object_t *value) {
    property_t *prop = calloc(sizeof(property_t), 1);
    prop->id = pid;
    prop->value.type = PT_OBJECT;
    prop->value.d.ptr = (void*)value;

    object_property_add_core(obj, prop);
}

void object_property_add_string(object_t *obj, int pid, const char *text) {
    property_t *prop = calloc(sizeof(property_t), 1);
    prop->id = pid;
    prop->value.type = PT_STRING;
    prop->value.d.ptr = (void*)text;

    object_property_add_core(obj, prop);
}

void object_property_delete(object_t *obj, int pid) {
    property_t *iter = obj->properties;

    if (!iter) return;

    if (iter->id == pid) {
        obj->properties = iter->next;
        property_free(iter);
        return;
    }

    while (iter->next && iter->next->id != pid) {
        iter = iter->next;
    }
    if (!iter->next) {
        return;
    }
    property_t *to_delete = iter->next;
    iter->next = iter->next->next;
    property_free(to_delete);
}

property_t* object_property_get(object_t *obj, int pid) {
    property_t *cur = obj->properties;

    while (cur) {
        if (cur->id == pid) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

void objectloop_depth_first(object_t *root, void (*callback)(object_t *obj)) {
    object_t *cur = root->first_child;
    while (cur) {
        callback(cur);
        objectloop_depth_first(cur, callback);
        cur = cur->sibling;
    }
}

void objectloop_free(object_t *obj) {

    object_t *child = obj->first_child;
    object_t *next_child = NULL;
    while (child) {
        next_child = child->sibling;
        objectloop_free(child);
        child = next_child;
    }

    object_free(obj);
}

void property_free(property_t *prop) {
    if (prop->value.type == PT_ARRAY) {
        free(prop->value.d.ptr);
    }
    free(prop);
}

