/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
    volatile uint32_t divstat[64];    /* 0x600 */
    volatile uint32_t gate[128];      /* 0x700 */
    volatile uint32_t clkout;         /* 0xA00 */
    volatile uint32_t spare[4];       /* 0xB00 */
};
