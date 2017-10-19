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

void object_free(object_t *obj) {
    property_t *cur = obj->properties;
    property_t *next = NULL;
    while (cur) {
        next = cur->next;
        object_free_property(cur);
        cur = next;
    }
    free(obj);
}

void object_free_property(property_t *prop) {
    if (prop->value.type == PT_ARRAY) {
        free(prop->value.d.ptr);
    }
    free(prop);
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
    property_t *cur = obj->properties;
    property_t *prev = NULL;

    while (cur) {
        if (cur == prop) return;
        if (cur->id == prop->id) {
            if (prev) {
                prev->next = cur->next;
            }
            if (cur == obj->properties) {
                obj->properties = cur->next;
            }
            property_t *tmp = cur->next;
            object_free_property(cur);
            cur = tmp;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

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