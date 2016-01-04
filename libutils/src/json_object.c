/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/json.h>

typedef struct _field {
    char *key;
    json_t *value;
    struct _field *next;
} field_t;

typedef struct _item {
    json_t *value;
    struct _item *next;
} item_t;

struct json {
    enum {
        BOOL,
        INT,
        UINT,
        POINTER,
        STRING,
        OBJECT,
        ARRAY,
    } type;
    union {
        bool bool_value;
        int int_value;
        unsigned int uint_value;
        void *pointer;
        char *string;
        field_t *object;
        item_t *array;
    };
    bool safe_strings;
};

static json_t *new(int type, bool safe) {
    json_t *j = calloc(1, sizeof(*j));
    if (j == NULL) {
        return NULL;
    }

    j->type = type;
    j->safe_strings = safe;
    return j;
}

json_t *json_new_bool(bool value) {
    json_t *j = new(BOOL, true);
    if (j != NULL) {
        j->bool_value = value;
    }
    return j;
}

json_t *json_new_int(int value) {
    json_t *j = new(INT, true);
    if (j != NULL) {
        j->int_value = value;
    }
    return j;
}

json_t *json_new_uint(unsigned int value) {
    json_t *j = new(UINT, true);
    if (j != NULL) {
        j->uint_value = value;
    }
    return j;
}

json_t *json_new_pointer(void *value) {
    json_t *j = new(POINTER, true);
    if (j != NULL) {
        j->pointer = value;
    }
    return j;
}

json_t *json_new_string(char *value, bool safe) {
    json_t *j = new(STRING, safe);
    if (j != NULL) {
        j->string = value;
    }
    return j;
}

json_t *json_new_object(bool safe) {
    return new(OBJECT, safe);
}

json_t *json_new_array(bool safe) {
    return new(ARRAY, safe);
}

int json_set_bool(json_t *j, bool value) {
    assert(j->type == BOOL);
    j->bool_value = value;
    return 0;
}

int json_set_int(json_t *j, int value) {
    assert(j->type == INT);
    j->int_value = value;
    return 0;
}

int json_set_uint(json_t *j, unsigned int value) {
    assert(j->type == UINT);
    j->uint_value = value;
    return 0;
}

int json_set_pointer(json_t *j, void *value) {
    assert(j->type == POINTER);
    j->pointer = value;
    return 0;
}

int json_add_field(json_t *j, char *key, json_t *value) {
    assert(j->type == OBJECT);
    field_t *f = malloc(sizeof(*f));
    if (f == NULL) {
        return -1;
    }
    f->key = key;
    f->value = value;
    f->next = j->object;
    j->object = f;
    return 0;
}

int json_update_field(json_t *j, char *key, json_t *value) {
    assert(j->type == OBJECT);
    for (field_t *f = j->object; f != NULL; f = f->next) {
        if (!strcmp(f->key, key)) {
            /* Found an existing matching field. */
            f->value = value;
            return 0;
        }
    }
    /* Didn't find a matching entry. */
    return json_add_field(j, key, value);
}

int json_append_item(json_t *j, json_t *item) {
    assert(j->type == ARRAY);
    item_t *i = malloc(sizeof(*i));
    if (i == NULL) {
        return -1;
    }
    i->value = item;
    i->next = j->array;
    j->array = i;
    return 0;
}

int json_print(json_t *j) {
    switch (j->type) {
        case BOOL:
            return json_print_bool(j->bool_value);
        case INT:
            return json_print_int(j->int_value);
        case UINT:
            return json_print_uint(j->uint_value);
        case STRING:
            if (j->safe_strings) {
                return json_print_safe_string(j->string);
            } else {
                return json_print_string(j->string);
            }
        case OBJECT: {
            int printed = 0;
            printed += printf("{");
            bool first = true;
            for (field_t *f = j->object; f != NULL; f = f->next) {
                if (!first) {
                    printed += printf(",");
                }
                printed += printf("%s:", f->key);
                printed += json_print(f->value);
                first = false;
            }
            printed += printf("}");
            return printed;
        }
        case ARRAY: {
            int printed = 0;
            printed += printf("[");
            bool first = true;
            for (item_t *i = j->array; i != NULL; i = i->next) {
                if (!first) {
                    printed += printf(",");
                }
                printed += json_print(i->value);
                first = false;
            }
            printed += printf("]");
            return printed;
        }
        default:
            assert(!"unknown type");
            return 0;
    }
}

void json_destroy(json_t *j) {
    switch (j->type) {
        case BOOL:
        case INT:
        case UINT:
        case POINTER:
        case STRING:
            free(j);
            break;
        case OBJECT:
            for (field_t *f = j->object; f != NULL;) {
                field_t *next = f->next;
                free(f);
                f = next;
            }
        case ARRAY:
            for (item_t *i = j->array; i != NULL;) {
                item_t *next = i->next;
                free(i);
                i = next;
            }
        default:
            assert(!"unknown type");
    }
}
