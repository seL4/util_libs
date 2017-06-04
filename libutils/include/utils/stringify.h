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

/* Macros related to string- and token-construction operations. */

#pragma once

/* See http://gcc.gnu.org/onlinedocs/cpp/Stringification.html for the purpose
 * of the extra level of indirection.
 */
#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

#define _JOIN(x, y) x ## y
#define JOIN(x, y) _JOIN(x, y)
