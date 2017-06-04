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

/* the NVIDIA Timers (TMR) in the manual, there are 10 of them for TK1
 * each timer has a 29-bit programmable timer counter and a 32-bit
 * timestamp counter
 * */

#include <platsupport/timer.h>
#include <stdint.h>

typedef enum {
    TMR0 = 0,
    TMR1,
    TMR2,
    TMR3,
    TMR4,
    TMR5,
    TMR6,
    TMR7,
    TMR8,
    TMR9,
    TMR_LAST = TMR9,
    PS_DEFAULT_TIMER = TMR0
} nv_tmr_id_t;

#define INT_NV_TMR0         188
#define INT_NV_TMR1         32
#define INT_NV_TMR2         33
#define INT_NV_TMR3         73
#define INT_NV_TMR4         74
#define INT_NV_TMR5         153
#define INT_NV_TMR6         184
#define INT_NV_TMR7         185
#define INT_NV_TMR8         186
#define INT_NV_TMR9         187

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


/* all timers are in on 4K page */
#define NV_TMR_PADDR    0x60005000
#define NV_TMR_SIZE     0x1000

typedef struct {
    void        *vaddr;
    void        *tmrus_vaddr;
    void        *shared_vaddr;
    uint32_t    irq;
} nv_tmr_config_t;

pstimer_t *nv_get_timer(nv_tmr_config_t *config);
