/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define OFFSET_SCLKCPLL    (0x20 / 4)
#define OFFSET_SCLKDPLL    (0x28 / 4)
#define OFFSET_SCLKEPLL    (0x30 / 4)
#define OFFSET_SCLKRPLL    (0x40 / 4)
#define OFFSET_SCLKIPLL    (0x50 / 4)
#define OFFSET_SCLKSPLL    (0x60 / 4)
#define OFFSET_SCLKVPLL    (0x70 / 4)
#define OFFSET_SCLKMPLL    (0x80 / 4)

#define CLKID_SCLKCPLL     CLKID(TOP, 8, 0)
#define CLKID_SCLKDPLL     CLKID(TOP, 10, 0)
#define CLKID_SCLKEPLL     CLKID(TOP, 12, 0)
#define CLKID_SCLKRPLL     CLKID(TOP, 16, 0)
#define CLKID_SCLKIPLL     CLKID(TOP, 20, 0)
#define CLKID_SCLDVPLL     CLKID(TOP, 24, 0)
#define CLKID_SCLKVPLL     CLKID(TOP, 28, 0)
#define CLKID_SCLKMPLL     CLKID(TOP, 32, 0)

static enum clk_id clk_src_peri_blk[] = {
    CLK_MASTER,
    CLK_SCLKCPLL,
    -1, //CLK_SCLKDPLL
    CLK_SCLKMPLL,
    -1, //CLK_SCKLSPLL
    -1, //CLK_SCKLIPLL
    CLK_SCLKEPLL,
    -1 //CLK_SCLKRPLL,
};


