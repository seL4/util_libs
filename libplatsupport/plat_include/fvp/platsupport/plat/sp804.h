/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/fdt.h>

/* Each SP804 has two timers, but we only use one timer on eace device page.
 * This is because the two timers on the same page share the same interrupt,
 * and using one timer on each page saves us from identifying the sources of
 * interrupts.
 * */
#define SP804_TIMER1_PATH "/soc/timer@1c110000"
#define SP804_TIMER2_PATH "/soc/timer@1c120000"

#define SP804_REG_CHOICE 0
#define SP804_IRQ_CHOICE 0

static UNUSED timer_properties_t sp804_timer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef volatile struct sp804_regs {
    uint32_t load;
    uint32_t value;
    uint32_t control;
    uint32_t intclr;
    uint32_t ris;
    uint32_t mis;
    uint32_t bgload;
} sp804_regs_t;

typedef struct {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* what are we being used for? */

    /* set in fdt helper */
    volatile sp804_regs_t *sp804_map;
    pmem_region_t pmem;
    irq_id_t irq_id;

    /* set in setup */
    uint32_t time_h;
} sp804_t;

typedef struct {
    const char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;
} sp804_config_t;

int sp804_init(sp804_t *sp804, ps_io_ops_t ops, sp804_config_t config);
/* convert between dmt ticks and ns */
uint64_t sp804_ticks_to_ns(uint64_t ticks);
/* return true if an overflow irq is pending */
bool sp804_is_irq_pending(sp804_t *sp804);
int sp804_set_timeout_ticks(sp804_t *timer, uint32_t ticks, bool periodic, bool irqs);
/* set a timeout in nano seconds */
int sp804_set_timeout(sp804_t *timer, uint64_t ns, bool periodic, bool irqs);
int sp804_start(sp804_t *timer);
int sp804_stop(sp804_t *timer);
uint64_t sp804_get_time(sp804_t *timer);
uint64_t sp804_get_ticks(sp804_t *timer);
void sp804_destroy(sp804_t *sp804);
