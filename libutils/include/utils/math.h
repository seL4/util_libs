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

#include <stdint.h>

/* This function can be used to calculate ((a * b) / c) without causing
 * overflow by doing (a * b). It also allows you to avoid loss of precision
 * if you attempte to rearrange the algorithm to (a * (c / b))
 * It is essentially the Ancient Egyption Multiplication
 * with (a * (b / c)) but we keep (b / c) in quotient+remainder
 * form to not lose precision.
 * This function only works with unsigned 64bit types. It can be
 * trivially abstracted to other sizes. Dealing with signed is possible
 * but not included here */
static inline uint64_t muldivu64(uint64_t a, uint64_t b, uint64_t c)
{
    /* quotient and remainder of the final calculation */
    uint64_t quotient = 0;
    uint64_t remainder = 0;
    /* running quotient and remainder of the current power of two
     * bit we are looking at. As we look at the different bits
     * of a we will increase these */
    uint64_t cur_quotient = b / c;
    uint64_t cur_remainder = b % c;
    /* we will iterate through all the bits of a from least to most
     * significant, we can stop early once there are no set bits though */
    while (a) {
        /* If this bit is set then the power of two is part of the
         * construction of a */
        if (a & 1) {
            /* increase final quotient, taking care of any remainder */
            quotient += cur_quotient;
            remainder += cur_remainder;
            if (remainder >= c) {
                quotient++;
                remainder -= c;
            }
        }
        /* Go to the next bit of a. Also means the effective quotient
         * and remainder of our (b / c) needs increasing */
        a >>= 1;
        cur_quotient <<= 1;
        cur_remainder <<= 1;
        /* Keep the remainder sensible, otherewise the remainder check
         * in the if (a & 1) case has to become a loop */
        if (cur_remainder >= c) {
            cur_quotient++;
            cur_remainder -= c;
        }
    }
    return quotient;
}
