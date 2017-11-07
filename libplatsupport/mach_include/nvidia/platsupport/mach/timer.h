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
    uintptr_t vaddr;
    nv_tmr_id_t id;
} nv_tmr_config_t;

struct tmr_map {
    uint32_t pvt;    /* present trigger value */
    uint32_t pcr;    /* present count value */
};

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

typedef struct nv_tmr {
    volatile struct tmr_map         *tmr_map;
    volatile struct tmrus_map       *tmrus_map;
    volatile struct tmr_shared_map  *tmr_shared_map;
    uint64_t                        counter_start;
} nv_tmr_t;

static UNUSED timer_properties_t tmr_properties = {
    .upcounter = true,
    .timeouts = true,
    .irqs = 1,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .absolute_timeouts = false,
};

int nv_tmr_start(nv_tmr_t *tmr);
int nv_tmr_stop(nv_tmr_t *tmr);
int nv_tmr_set_timeout(nv_tmr_t *tmr, bool periodic, uint64_t ns);
void nv_tmr_handle_irq(nv_tmr_t *tmr);
uint64_t nv_tmr_get_time(nv_tmr_t *tmr);
long nv_tmr_get_irq(nv_tmr_id_t n);
int nv_tmr_init(nv_tmr_t *tmr, nv_tmr_config_t config);
uint32_t nv_tmr_get_usec_upcounter_val(nv_tmr_t *tmr);
