/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
static inline void autofree_(void *p)
{
    void **q = (void **)p;
    free(*q);
}
#define AUTOFREE __attribute__((cleanup(autofree_)))
