/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define MCR(cpreg, v)                               \
    do {                                            \
        uint32_t _v = v;                            \
        asm volatile("mcr  " cpreg :: "r" (_v));    \
    } while(0)
#define MCRR(cpreg,v)                               \
    do {                                            \
        uint64_t _v = v;                            \
        asm volatile("mcrr  " cpreg :: "r" (_v));     \
    } while (0)
#define MRRC(cpreg, v)  asm volatile("mrrc " cpreg :  "=r"(v))
#define MRC(cpreg, v)  asm volatile("mrc   " cpreg :  "=r"(v))

#define COPROC_WRITE_WORD(R,W) MCR(R,W)
#define COPROC_READ_WORD(R,W)  MRC(R,W)
#define COPROC_WRITE_64(R,W)   MCRR(R,W)
#define COPROC_READ_64(R,W)    MRRC(R,W)

/* control reigster for the el1 physical timer */
#define CNTP_CTL  " p15, 0,  %0, c14,  c2, 1"
/* holds the compare value for the el1 physical timer */
#define CNTP_CVAL " p15, 2, %Q0, %R0, c14   "
/* holds the 64-bit physical count value */
#define CNTPCT    " p15, 0, %Q0, %R0, c14" /* 64-bit RO Physical Count register */
/* frequency of the timer */
#define CNTFRQ    " p15, 0,  %0, c14,  c0, 0" /* 32-bit RW Counter Frequency register */
