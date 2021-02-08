/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* the NVIDIA Timers (TMR) in the manual, there are 10 of them for TK1
 * each timer has a 29-bit programmable timer counter and a 32-bit
 * timestamp counter
 * */

#include <stdint.h>

#define TMR0_OFFSET         0x88
#define TMR1_OFFSET         0x00
#define TMR2_OFFSET         0x08
#define TMR3_OFFSET         0x50
#define TMR4_OFFSET         0x58
#define TMR5_OFFSET         0x60
#define TMR6_OFFSET         0x68
#define TMR7_OFFSET         0x70
#define TMR8_OFFSET         0x78
#define TMR9_OFFSET         0x80
#define TMRUS_OFFSET        0x10
#define TMR_SHARED_OFFSET   0x1a0

#define NV_TMR_PATH "/timer@60005000"
#define NV_TMR_ID TMR1
#define NV_TMR_ID_OFFSET TMR1_OFFSET

struct tmr_shared_map {
    uint32_t intr_status;
    uint32_t secure_cfg;
};

struct tmrus_map {
    /* A free-running read-only counter changes once very microsecond */
    uint32_t cntr_1us;
    /* configure this regsiter by telling what fraction of 1 microsecond
     * each clk_m represents. if the clk_m is running at 12 MHz, then
     * each clm_m represent 1/12 of a microsecond.*/
    uint32_t usec_cfg;
    uint32_t cntr_freeze;
};
