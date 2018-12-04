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

#define OFFSET_SCLKCPLL    (0x20 / 4)
#define OFFSET_SCLKEPLL    (0x30 / 4)
#define OFFSET_SCLKVPLL    (0x40 / 4)
#define OFFSET_SCLKGPLL    (0x50 / 4)
#define OFFSET_SCLKMPLL    (0x00 / 4)
#define OFFSET_SCLKBPLL    (0x10 / 4)

#define CLKID_SCLKCPLL     CLKID(TOP, 6, 2)
#define CLKID_SCLKEPLL     CLKID(TOP, 6, 3)
#define CLKID_SCLKVPLL     CLKID(TOP, 6, 4)
#define CLKID_SCLKGPLL     CLKID(TOP, 6, 7)
#define CLKID_SCLKMPLL     CLKID(CORE, 0, 2)
#define CLKID_SCLKBPLL     CLKID(CDREX, 0, 0)

static enum clk_id clk_src_peri_blk[] = {
                                            CLK_MASTER,
                                            CLK_MASTER,
                                            -1, // CLK_HDMI27M
                                            -1, // CLK_DPTXPHY
                                            -1, // CLK_UHOSTPHY
                                            -1, // CLK_HDMIPHY
                                            CLK_SCLKMPLL,
                                            CLK_SCLKEPLL,
                                            CLK_SCLKVPLL,
                                            CLK_SCLKCPLL
                                        };


