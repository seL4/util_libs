/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_BUILTIN_H
#define _UTILS_BUILTIN_H
/* macros for accessing compiler builtins */

#define CTZ(x) __builtin_ctz(x)
#define CLZ(x) __builtin_clz(x)
#define OFFSETOF(type, member) __builtin_offsetof(type, member)
#define TYPES_COMPATIBLE(t1, t2) __builtin_types_compatible_p(t1, t2)
#define CHOOSE_EXPR(cond, x, y) __builtin_choose_expr(cond, x, y)
#define IS_CONSTANT(expr) __builtin_constant_p(expr)
#define UNREACHABLE()  __builtin_unreachable()

#endif /* _UTILS_BUILTIN_H */
