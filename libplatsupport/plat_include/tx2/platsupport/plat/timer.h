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

/* the NVIDIA Timers (TMR) in the manual, there are 10 of them for TK1
 * each timer has a 29-bit programmable timer counter and a 32-bit
 * timestamp counter
 * */

#include <stdint.h>

#define TMR0_OFFSET         0x10000
#define TMR1_OFFSET         0x20000
#define TMR2_OFFSET         0x30000
#define TMR3_OFFSET         0x40000
#define TMR4_OFFSET         0x50000
#define TMR5_OFFSET         0x60000
#define TMR6_OFFSET         0x70000
#define TMR7_OFFSET         0x80000
#define TMR8_OFFSET         0x90000
#define TMR9_OFFSET         0xA0000
#define TMRUS_OFFSET        0x8
#define TMR_SHARED_OFFSET   0

#define NV_TMR_PATH "/timer@3020000"
#define NV_TMR_ID TMR0
#define NV_TMR_ID_OFFSET TMR0_OFFSET

struct tmr_shared_map {
    uint32_t TKETSC0; // Value of local TSC counter TSC[31:0], synchronized across SOC
    uint32_t TKETSC1; // Value of local TSC counter {8’h0, TSC[55:32]}, synchronized across SOC
    uint32_t TKEUSEC; // This is the same as the tmrus_map.cntr_1us below
    uint32_t TKEOSC;  // Value of local OSC counter, not synchronized across SOC
    uint32_t TKECR;   // Control register
    uint32_t pad[(0x100/4)-5]; // Have to pad to 0x100
    uint32_t TKEIE[10]; // Routing of shared interrupt {i}, a bit mask indicating which
                        // of the internal interrupts is propagated to external interrupt {i},
    uint32_t pad1[(0x100/4)-10]; // Have to pad to 0x200
    uint32_t TKEIV;   // Which shared interrupts are currently asserted
    uint32_t TKEIR;   // Which internal interrupts are currently asserted, before applying the
                      // TKEIE masks
} PACKED;
static_assert(sizeof(struct tmr_shared_map) == 0x208, "struct tmr_shared_map has incorrect layout");

/* A free-running read-only counter changes once very microsecond.
   On TX2 this is just a register in the shared Timer region. */
struct tmrus_map {
    uint32_t cntr_1us; /* offset 0 */
};

#include <platsupport/mach/timer.h>
