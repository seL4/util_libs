/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <utils/list.h>
#include <utils/attribute.h>

typedef struct list_node node_t;

int list_init(list_t *l)
{
    assert(l != NULL);
    l->head = NULL;
    return 0;
}

static node_t *make_node(void *data)
{
    node_t *n = malloc(sizeof(*n));
    if (n != NULL) {
        n->data = data;
        n->next = NULL;
    }
    return n;
}

int list_prepend(list_t *l, void *data)
{
    node_t *n = make_node(data);
    if (n == NULL) {
        return -1;
    }
    return list_prepend_node(l, n);
}

int list_append(list_t *l, void *data)
{
    node_t *n = make_node(data);
    if (n == NULL) {
        return -1;
    }
    return list_append_node(l, n);
}

bool list_is_empty(list_t *l)
{
    assert(l != NULL);
    return l->head == NULL;
}

bool list_exists(list_t *l, void *data, int(*cmp)(void *, void *))
{
    assert(l != NULL);
    for (node_t *n = l->head; n != NULL; n = n->next) {
        if (!cmp(n->data, data)) {
            return true;
        }
    }
    return false;
}

int list_length(list_t *l)
{
    assert(l != NULL);
    int i = 0;
    for (node_t *n = l->head; n != NULL; n = n->next) {
        i++;
    }
    return i;
}

int list_index(list_t *l, void *data, int(*cmp)(void *, void *))
{
    assert(l != NULL);
    int i = 0;
    for (node_t *n = l->head; n != NULL; n = n->next, i++) {
        if (!cmp(n->data, data)) {
            return i;
        }
    }
    return -1;
}

int list_foreach(list_t *l, int(*action)(void *, void *), void *token)
{
    assert(l != NULL);
    for (node_t *n = l->head; n != NULL; n = n->next) {
        int res = action(n->data, token);
        if (res != 0) {
            return res;
        }
    }
    return 0;
}

static int remove(list_t *l, void *data, int (*cmp)(void *, void *),
                  bool should_free)
{
    assert(l != NULL);
    for (node_t *prev = NULL, *n = l->head; n != NULL; prev = n, n = n->next) {
        if (!cmp(n->data, data)) {
            if (prev == NULL) {
                /* Removing the list head. */
                l->head = n->next;
            } else {
                prev->next = n->next;
            }
            if (should_free) {
                free(n);
            }
            return 0;
        }
    }
    return -1;
}

int list_remove(list_t *l, void *data, int(*cmp)(void *, void *))
{
    return remove(l, data, cmp, true);
}

int list_remove_all(list_t *l)
{
    assert(l != NULL);
    for (node_t *n = l->head; n != NULL;) {
        node_t *temp = n->next;
        free(n);
        n = temp;
    }
    l->head = NULL;
    return 0;
}

int list_destroy(UNUSED list_t *l)
{
    /* Nothing required. */
    return 0;
}

int list_prepend_node(list_t *l, node_t *node)
{
    assert(l != NULL);
    assert(node != NULL);
    node->next = l->head;
    l->head = node;
    return 0;
}

int list_append_node(list_t *l, node_t *node)
{
    assert(l != NULL);
    assert(node != NULL);
    node->next = NULL;
    if (l->head == NULL) {
        l->head = node;
    } else {
        node_t *end;
        for (end = l->head; end->next != NULL; end = end->next);
        end->next = node;
    }
    return 0;
}

int list_remove_node(list_t *l, void *data, int(*cmp)(void *, void *))
{
    return remove(l, data, cmp, false);
}

int list_remove_all_nodes(list_t *l)
{
    assert(l != NULL);
    l->head = NULL;
    return 0;
}
