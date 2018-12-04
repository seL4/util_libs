/*
 * Copyright 2018, Data61
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

#include <inttypes.h>

struct clk_regs {
    volatile uint32_t pll_lock[64];   /* 0x000 */
    volatile uint32_t pll_con[64];    /* 0x100 */
    volatile uint32_t src[64];        /* 0x200 */
    volatile uint32_t srcmask[64];    /* 0x300 */
    volatile uint32_t srcstat[64];    /* 0x400 */
    volatile uint32_t div[64];        /* 0x500 */
    volatile uint32_t divstat[128];   /* 0x600 */
    volatile uint32_t gate[64];       /* 0x800 */
    volatile uint32_t clkout;         /* 0xA00 */
    volatile uint32_t clkout_divstat; /* 0xA04 */
};
