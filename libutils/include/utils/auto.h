/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

/* Helpers for resource management. */

#include <stdlib.h>

/* Support for automatically freed heap pointers. This function is never
 * expected to be called directly; you are expected to access it through the
 * adjacent macro. Sample usage:
 *
 *     void foo(void) {
 *       autofree int *x = malloc(sizeof(int));
 *       // no need to call free(x)
 *     }
 */
static inline void autofree_(void *p) {
    void **q = (void**)p;
    free(*q);
}
#define AUTOFREE __attribute__((cleanup(autofree_)))
