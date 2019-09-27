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

#pragma once

/* macros for doing basic math */

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <utils/attribute.h>
#include <utils/builtin.h>
#include <stdint.h>
#include <utils/verification.h>
#include <utils/stringify.h>

#define BIT(n) (1ul<<(n))

#define MASK_UNSAFE(x) ((BIT(x) - 1ul))

/* The MASK_UNSAFE operation involves using BIT that performs a left shift, this
 * shift is only defined by the C standard if shifting by 1 less than the
 * number of bits in a word. MASK allows both the safe creation of masks, and for
 * creating masks that are larger than what is possible with MASK_UNSAFE, as
 * MASK_UNSAFE cannot create a MASK that is all 1's */
#define MASK(n) \
    ({  typeof (n) _n = (n); \
        (void)assert(_n <= (sizeof(unsigned long) * 8)); \
        (void)assert(_n > 0); \
        MASK_UNSAFE(_n - 1) | BIT(_n - 1); \
    })

#define IS_ALIGNED(n, b) (!((n) & MASK(b)))

/* Calculate the log2 by finding the most significant bit that is set.
 * We have CLZL, which tells us how many places from the 'left' it is,
 * and by subtracting that from the number of bits in the word (minus 1)
 * we learn how many bits from the 'right' it is. */
#define LOG_BASE_2(n) (sizeof(unsigned long) * CHAR_BIT - CLZL(n) - 1)

/* Taken from Hacker's Delight, this works on 32 bit integers,
 * also note that it *will* calculate the *next* power of two,
 * i.e. NEXT_POWER_OF_2(64) == 128 not 64 */
#define NEXT_POWER_OF_2(x)  \
    ({ uint32_t temp = (uint32_t) x;    \
       temp |= (temp >> 1);             \
       temp |= (temp >> 2);             \
       temp |= (temp >> 4);             \
       temp |= (temp >> 8);             \
       temp |= (temp >> 16);            \
       temp += 1;                       \
    })

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
       (_n + (_n % _b == 0 ? 0 : (_b - (_n % _b)))); \
    })

#define DIV_ROUND_UP(n,d)   \
    ({ typeof (n) _n = (n); \
       typeof (d) _d = (d); \
       (_n/_d + (_n % _d == 0 ? 0 : 1)); \
   })

/* Divides and rounds to the nearest whole number
    DIV_ROUND(5,2) returns 3
    DIV_ROUND(7,3) returns 2 */
#define DIV_ROUND_UNSAFE(n,d)  \
    ({ typeof (n) _n = (n); \
       typeof (d) _d = (d); \
       ((_n + (_d/2)) / _d); \
   })

#define DIV_ROUND(n,d)  \
   ({ typeof (n) _n = (n); \
      typeof (d) _d = (d); \
      ((_n / _d) + ((_n % _d) >= _d/2 ? 1 : 0)); \
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

/* Clamp a value between two limits. Sample usage: CLAMP(-3, 7, 4) == 4. */
#define CLAMP(min, value, max) \
    ({  typeof (max) _max = (max); \
        typeof (min) _min = (min); \
        typeof (value) _value = (value); \
        if (_value > _max) { \
            _value = _max; \
        } else if (_value < _min) { \
            _value = _min; \
        } \
        _value;  })

/* Clamp an addition. Sample usage: CLAMP_ADD(1<<30, 1<<30, INT_MAX) == INT_MAX. */
#define CLAMP_ADD(operand1, operand2, limit) \
    ({  typeof (operand1) _op1 = (operand1); \
        typeof (operand2) _op2 = (operand2); \
        typeof (limit) _limit = (limit); \
        _limit - _op2 < _op1 ? _limit : _op1 + _op2;  })

/* Clamp a subtraction. Sample usage:
 *   CLAMP_SUB(-1 * (1<<30), -1 * (1<<30), INT_MIN) == INT_MIN.
 */
#define CLAMP_SUB(operand1, operand2, limit) \
    ({  typeof (operand1) _op1 = (operand1); \
        typeof (operand2) _op2 = (operand2); \
        typeof (limit) _limit = (limit); \
        _limit + _op2 > _op1 ? _limit : _op1 - _op2;  })

/* Expands to a struct field declaration suitable for padding structs where each
 * field must have a specific offset (such as in device drivers). E.g.:
 * struct device_registers {
 *      uint32_t reg1; // 0x00
 *      uint32_t reg2; // 0x04
 *      PAD_BETWEEN(0x04, 0x10, uint32_t); // expands to: uint8_t __padding4[8];
 *      uint32_t reg3; // 0x10
 * } PACKED;
 *
 * Using this macro in a struct requires that struct to be defined with
 * the "packed" attribute.
 *
 * @param before    offset of struct field before padding
 * @param after     offset of struct field after padding
 * @param type      type of field before padding
 */
#define PAD_STRUCT_BETWEEN(before, after, type) \
        uint8_t JOIN(__padding, __COUNTER__)[(after) - (before) - sizeof(type)]
