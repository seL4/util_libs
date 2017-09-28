/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <platsupport/io.h>
#include <stdlib.h>

static int ps_stdlib_malloc(UNUSED void *cookie, size_t size, void **ptr)
{
    assert(ptr != NULL);
    *ptr = malloc(size);
    if (*ptr == NULL) {
        return ENOMEM;
    }
    return 0;
}

static int ps_stdlib_calloc(UNUSED void *cookie, size_t nmemb, size_t size, void **ptr)
{
    assert(ptr != NULL);
    *ptr = calloc(nmemb, size);
    if (*ptr == NULL) {
        return ENOMEM;
    }
    return 0;
}

static int ps_stdlib_free(UNUSED void *cookie, UNUSED size_t size, void *ptr)
{
    free(ptr);
    return 0;
}

int ps_new_stdlib_malloc_ops(ps_malloc_ops_t *ops)
{
    ops->malloc = ps_stdlib_malloc;
    ops->calloc = ps_stdlib_calloc;
    ops->free = ps_stdlib_free;
    ops->cookie = NULL;
    return 0;
}
