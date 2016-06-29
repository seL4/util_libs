/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ASSEMBLER_32_V7_H__
#define __ASSEMBLER_32_V7_H__

#include "assembler.h"

/* This file contains useful macros for assembly code. */

#ifdef __ASSEMBLER__

#define     CCSIDR(reg)    p15, 1, reg, c0, c0, 0
#define     CSSELR(reg)    p15, 2, reg, c0, c0, 0
#define      ACTLR(reg)    p15, 0, reg, c1, c0, 1
#define       DISW(reg)    p15, 0, reg, c7, c6, 2
#define      DCISW(reg)    p15, 0, reg, c7, c14, 2

.macro dcache op
    dmb
    mrc     CLIDR(r0)
    mov     r3, r0, lsr #23             //move LoC into position
    ands    r3, r3, #7 << 1             //extract LoC*2 from clidr

    beq     finished_\op                //if loc is 0, then no need to clean
    mov     r10, #0                     //start clean at cache level 0

loop1_\op:
    add     r2, r10, r10, lsr #1        //work out 3x current cache level
    mov     r1, r0, lsr r2              //extract cache type bits from clidr
    and     r1, r1, #7                  //mask of the bits for current cache only
    cmp     r1, #2                      //see what cache we have at this level
    blt     skip_\op                    //skip if no cache, or just i-cache

    mcr     CSSELR(r10)
    isb

    mrc     CCSIDR(r1)
    and     r2, r1, #7                  //extract the length of the cache lines
    add     r2, r2, #4                  //add 4 (line length offset)
    movw    r4, #0x3ff
    ands    r4, r4, r1, lsr #3          //find maximum number on the way size
    clz     r5, r4                      //find bit position of way size increment
    movw    r7, #0x7fff
    ands    r7, r7, r1, lsr #13         //extract max number of the index size

loop2_\op:
    mov     r9, r7                      //create working copy of max index

loop3_\op:
    orr     r11, r10, r4, lsl r5        //factor way and cache number into r11
    orr     r11, r11, r9, lsl r2        //factor index number into r11
.if \op == isw
    mcr     DISW(r11)
.elseif \op == cisw
    mcr     DCISW(r11)
.endif
    subs    r9, r9, #1                  //decrement the index
    bge     loop3_\op
    subs    r4, r4, #1                  //decrement the way
    bge     loop2_\op

skip_\op:
    add     r10, r10, #2                //increment cache number
    cmp     r3, r10
    bgt     loop1_\op

finished_\op:
    mov     r10, #0                     //swith back to cache level 0
    mcr     p15, 2, r10, c0, c0, 0      //select current cache level in cssr
    dsb     st
    isb
.endm

#else /* !__ASSEMBLER__ */
#warning "Including assembly-specific header in C code"
#endif

#endif /* __ASSEMBLER_32_V7_H__ */
