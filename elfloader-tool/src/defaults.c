/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* Default implementations of required utility functions. Override these under
 * plat-* if there is a more appropriate implementation for a given platform.
 */

#include <printf.h>

void __attribute__((weak)) abort(void)
{
    printf("abort() called.\n");

    while (1);
}
