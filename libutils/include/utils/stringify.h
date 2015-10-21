/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Macros related to string- and token-construction operations. */

#ifndef _UTILS_STRINGIFY_H
#define _UTILS_STRINGIFY_H

/* See http://gcc.gnu.org/onlinedocs/cpp/Stringification.html for the purpose
 * of the extra level of indirection.
 */
#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

#define _JOIN(x, y) x ## y
#define JOIN(x, y) _JOIN(x, y)

#endif
