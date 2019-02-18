/*
 * Copyright 2019, Data61
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

#include <platsupport/timer.h>

/* Each SP804 has two timers, but we only use one timer on eace device page.
 * This is because the two timers on the same page share the same interrupt,
 * and using one timer on each page saves us from identifying the sources of
 * interrupts.
 * */
#define SP804_0_PADDR 0x1c110000
#define SP804_1_PADDR 0x1c120000

/* IRQs */
#define SP804_0_INTERRUPT   34
#define SP804_1_INTERRUPT   35

/* Timers */
typedef enum timer_id {
    SP804_TIMER0,
    SP804_TIMER1,
    NUM_SP804_TIMERS = 2,
} sp804_id_t;

#define TMR_DEFAULT DMTIMER0

static const uintptr_t sp804_timer_paddrs[] = {
    [SP804_TIMER0] = SP804_0_PADDR,
    [SP804_TIMER1] = SP804_1_PADDR,
};

static const long sp804_timer_irqs[] = {
    [SP804_TIMER0] = SP804_0_INTERRUPT,
    [SP804_TIMER1] = SP804_1_INTERRUPT,
};

static inline void *sp804_get_paddr(sp804_id_t id)
{
    if (id < NUM_SP804_TIMERS && id >= SP804_TIMER0) {
        return (void *) sp804_timer_paddrs[id];
    }
    return NULL;
}

static inline long sp804_get_irq(sp804_id_t id)
{
    if (id < NUM_SP804_TIMERS && id >= SP804_TIMER0) {
        return sp804_timer_irqs[id];
    }
    return 0;
}

static UNUSED timer_properties_t sp804_timer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    void *regs;
} sp804_t;

typedef struct {
    void *vaddr;
    int id;
} sp804_config_t;

int sp804_init(sp804_t *timer, sp804_config_t config);
void sp804_handle_irq(sp804_t *timer);
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
