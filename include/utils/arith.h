/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_ARITH_H
#define _UTILS_ARITH_H
/* macros for doing basic math */

#include <assert.h>
#include <stdint.h>
#include <utils/attribute.h>
#include <stdint.h>

#define BIT(n) (1ul<<(n))

#define MASK_UNSAFE(x) ((BIT(x) - 1ul))

#define MASK(n) ({(void)assert((n) <= 31); MASK_UNSAFE(n); })

#define IS_ALIGNED(n, b) (!((n) & MASK(b)))

#define IS_POWER_OF_2_OR_ZERO(x) (0 == ((x) & ((x) - 1)))
#define IS_POWER_OF_2(x) (((x) != 0) && IS_POWER_OF_2_OR_ZERO(x))
#define ALIGN_UP(x, n) (((x) + (n) - 1) & ~((n) - 1))
#define ALIGN_DOWN(x, n) ((x) & ~((n) - 1))

#define ROUND_DOWN_UNSAFE(n, b) ((n) - ((n) % (b)))

#define ROUND_DOWN(n, b) \
    ({ typeof (n) _n = (n); \
       typeof (b) _b = (b); \
       _n - (_n % _b); \
    })

#define ROUND_UP_UNSAFE(n, b) ((n) + ((n) % (b) == 0 ? 0 : ((b) - ((n) % (b)))))

#define ROUND_UP(n, b) \
    ({ typeof (n) _n = (n); \
       typeof (b) _b = (b); \
       (_n + (_n % b == 0 ? 0 : (_b - (_n % _b)))); \
    })

#define MIN(a,b) \
    ({ typeof (a) _a = (a); \
       typeof (b) _b = (b); \
       _a < _b ? _a : _b; })

/* used for compile time eval */
#define MIN_UNSAFE(x, y) ((x) < (y) ? (x) : (y))

#define MAX(a,b) \
    ({ typeof (a) _a = (a); \
       typeof (b) _b = (b); \
       _a > _b ? _a : _b; })

/* used for complile time eval */
#define MAX_UNSAFE(x, y) ((x) > (y) ? (x) : (y))

#define INRANGE(a, x, b) MIN(MAX(x, a), b)
#define ISINRANGE(a, x, b) \
    ({ typeof (x) _x = (x); \
       _x == INRANGE(a, _x, b); })

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#endif /* _UTILS_ARITH_H */
