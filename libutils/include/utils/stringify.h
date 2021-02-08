/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
