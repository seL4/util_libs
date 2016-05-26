/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Bog standard singly-linked-list implementation. */

#pragma once

#include <stdbool.h>

/* Type of a linked-list. */
typedef struct {
    struct list_node *head;
} list_t;

/* Create a new linked-list. Returns 0 on success. */
int list_init(list_t *l);

/* Prepend a value to the list. In general, prefer this to appending if order
 * of the elements is not relevant as prepending is faster. Returns 0 on
 * success.
 */
int list_prepend(list_t *l, void *data);

/* Append a value to the list. Returns 0 on success. */
int list_append(list_t *l, void *data);

/* Returns true if the given list contains no elements. */
bool list_is_empty(list_t *l);

/* Returns true if the given element is in the list. The third argument is a
 * comparator to determine list element equality.
 */
bool list_exists(list_t *l, void *data, int(*cmp)(void*, void*));

/* Returns the number of elements in the list. */
int list_length(list_t *l);

/* Returns the index of the given element in the list or -1 if the element is
 * not found.
 */
int list_index(list_t *l, void *data, int(*cmp)(void*, void*));

/* Call the given function on every list element. While traversing the list, if
 * the caller's action ever returns non-zero the traversal is aborted and that
 * value is returned. If traversal completes, this function returns 0.
 */
int list_foreach(list_t *l, int(*action)(void*));

/* Remove the given element from the list. Returns non-zero if the element is
 * not found.
 */
int list_remove(list_t *l, void *data, int(*cmp)(void*, void*));

/* Remove all elements from the list. Returns 0 on success. */
int list_remove_all(list_t *l);

/* Destroy the list. The caller is expected to have previously removed all
 * elements of the list. Returns 0 on success.
 */
int list_destroy(list_t *l);

/* Below various equivalents of operations above are provided, but that take a
 * caller-constructed node. The purpose of these is to allow you to
 * stack-/statically-allocate nodes when you have no dynamic memory.
 */

/* Internal structure of the node of list. */
struct list_node {
    void *data;
    struct list_node *next;
};

int list_prepend_node(list_t *l, struct list_node *node);
int list_append_node(list_t *l, struct list_node *node);
int list_remove_node(list_t *l, void *data, int(*cmp)(void*, void*));
int list_remove_all_nodes(list_t *l);
