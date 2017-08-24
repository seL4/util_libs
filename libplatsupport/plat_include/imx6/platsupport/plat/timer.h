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

/* Default ipg_clk configuration:
 *
 * FIN (24 MHZ)
 *  |_ *22 PLL2 (528)
 *           |_ /4 AHB_CLK (132 MHZ)
 *                     |_ /2 IPG_CLK (66 MHZ)
 *                              |_ EPIT
 */
#define PLL2_FREQ  (528u)
#define AHB_FREQ   (PLL2_FREQ / 4u)
#define IPG_FREQ   (AHB_FREQ  / 2u)
#define GPT_FREQ   IPG_FREQ

#include <platsupport/mach/gpt.h>
#include <platsupport/mach/epit.h>
