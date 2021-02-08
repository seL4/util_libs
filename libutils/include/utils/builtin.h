/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* macros for accessing compiler builtins */

#include <assert.h>

#define CTZ(x) __builtin_ctz(x)
#define CLZ(x) __builtin_clz(x)
#define CTZL(x) __builtin_ctzl(x)
#define CLZL(x) __builtin_clzl(x)
#define FFS(x) __builtin_ffs(x)
#define FFSL(x) __builtin_ffsl(x)
#define OFFSETOF(type, member) __builtin_offsetof(type, member)
#define TYPES_COMPATIBLE(t1, t2) __builtin_types_compatible_p(t1, t2)
#define CHOOSE_EXPR(cond, x, y) __builtin_choose_expr(cond, x, y)
#define IS_CONSTANT(expr) __builtin_constant_p(expr)
#define POPCOUNT(x) __builtin_popcount(x)
#define POPCOUNTL(x) __builtin_popcountl(x)

#if CONFIG_WORD_SIZE == 32
#define BSWAP_WORD(x) __builtin_bswap32(x)
#elif CONFIG_WORD_SIZE == 64
#define BSWAP_WORD(x) __builtin_bswap64(x)
#endif

/* The UNREACHABLE macro is used in some code that is imported to Isabelle via
 * the C-to-Simpl parser. This parser can handle calls to uninterpreted
 * functions, but it needs to see a function declaration before the call site,
 * as it is unaware of this compiler builtin. We also provide a spec for the
 * function indicating that any execution that reaches it represents failure.
 */
/** MODIFIES:
    FNSPEC
        builtin_unreachable_spec: "\<Gamma> \<turnstile> {} Call StrictC'__builtin_unreachable'_proc {}"
*/
void __builtin_unreachable(void);

#define UNREACHABLE() \
    do { \
        assert(!"unreachable"); \
        __builtin_unreachable(); \
    } while (0)

/* Borrowed from linux/include/linux/compiler.h */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
