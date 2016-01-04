/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Basic functionality for printing JSON data. */

#ifndef UTILS_JSON_H_
#define UTILS_JSON_H_

#include <stdbool.h>
#include <stdlib.h>

/* Print a beginning or ending marker for JSON output, respectively. There's
 * nothing JSON specific about what these functions do, but they are included
 * here for the purpose of standardised beginning and ending markers in mixed
 * console output including output from this library.
 */
int json_print_begin(void);
int json_print_end(void);

/*****
 * Low level interface. The following functions allow you to directly print C
 * primitives, for quick and dirty JSON output. These functions, unlike the
 * high level interface below, do not do any dynamic memory allocation.
 *****/

/* Functions for printing various primitives. */
int json_print_true(void);
int json_print_false(void);
int json_print_bool(bool v);
int json_print_null(void);
int json_print_int(int v);
int json_print_uint(unsigned int v);
int json_print_pointer(void *v);

/* Print a string. The safe version prints a human-readable version and assumes
 * there are no control characters in the argument.
 */
int json_print_string(char *s);
int json_print_safe_string(char *s);

/* Print an array. The caller is expected to provide a callback that determines
 * how to print an item of the array.
 *  E.g. json_print_array(my_ints, 10, json_print_int);
 */
int json_print_array(void **array, size_t size, int (*printer)(void *item));

/* Print an object. The caller is expected to provide an array of keys (fields)
 * and a callback for printing any particular field. The safe version prints
 * human-readable fields and assumes there are no control characters in the
 * keys.
 */
int json_print_safe_object(void *object, char **keys, size_t keys_sz,
    int (*printer)(void *object, char *key));
int json_print_object(void *object, char **keys, size_t keys_sz,
    int (*printer)(void *object, char *key));

/*****
 * High level interface. The following functions allow you to construct JSON
 * representations, manipulate them and add data as necessary, then finally
 * dump the structure to stdout.
 *****/

/* Opaque type of a JSON artefact. */
typedef struct json json_t;

/* Create a new JSON artefact of a given type. */
json_t *json_new_bool(bool value);
json_t *json_new_int(int value);
json_t *json_new_uint(unsigned int value);
json_t *json_new_pointer(void *value);
json_t *json_new_string(char *value, bool safe);
json_t *json_new_object(bool safe);
json_t *json_new_array(bool safe);

/* Set the value of a JSON artefact. The type of the artefact must correspond
 * to the function you are using to operate on it.
 */
int json_set_bool(json_t *j, bool value);
int json_set_int(json_t *j, int value);
int json_set_uint(json_t *j, unsigned int value);
int json_set_pointer(json_t *j, void *value);

/* Set the value of a field in a JSON object. The 'add' function is provided as
 * an optimisation if you know the field you're setting does not already exist.
 */
int json_add_field(json_t *j, char *key, json_t *value);
int json_update_field(json_t *j, char *key, json_t *value);

/* Add an item to a JSON array. */
int json_append_item(json_t *j, json_t *item);

/* Dump a JSON artefact to stdout. */
int json_print(json_t *j);

/* Deallocate resources associated with a JSON artefact. The caller should not
 * attempt to access the artefact after calling this function.
 */
void json_destroy(json_t *j);

#endif
