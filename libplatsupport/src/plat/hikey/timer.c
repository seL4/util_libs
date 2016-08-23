/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <assert.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define TCLR_ONESHOT	BIT(0)
#define TCLR_VALUE_32	BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)

#define TICKS_PER_SECOND 19200000
#define TIMER_INTERVAL_TICKS(ns) ((uint32_t)(1ULL * (ns) * TICKS_PER_SECOND / 1000 / 1000 / 1000))
#define TICKS_TIMER_INTERVAL(x)  (((uint64_t)x * 1000 * 1000 * 1000) / TICKS_PER_SECOND)

struct dmt_map {
	uint32_t load;
	uint32_t value;
	uint32_t control;
	uint32_t intclr;
	uint32_t ris;
	uint32_t mis;
	uint32_t bgload;	
};

typedef struct dmt {
    volatile struct dmt_map *hw;
    uint32_t irq;
} dmt_t;

static void
dm_timer_reset(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->control = 0;
}

static int
dm_timer_stop(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->control = dmt->hw->control & ~TCLR_STARTTIMER;

    return 0;
}

static int
dm_timer_start(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->control = dmt->hw->control | TCLR_STARTTIMER;

    return 0;
}

static int
dm_set_timeo(const pstimer_t *timer, uint64_t ns, int otherFlags)
{
    dmt_t *dmt = (dmt_t*) timer->data;

    dmt->hw->control = 0;

    uint32_t ticks = TIMER_INTERVAL_TICKS(ns);
    if (ticks < 2) {
        return EINVAL;
    }

    dmt->hw->load = ticks;
    dmt->hw->value = ticks;
    dmt->hw->control = TCLR_STARTTIMER | TCLR_INTENABLE | TCLR_VALUE_32 | otherFlags;

    return 0;
}

static int
dm_periodic(const pstimer_t *timer, uint64_t ns)
{
    return dm_set_timeo(timer, ns, TCLR_AUTORELOAD);
}

static int
dm_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    return ENOSYS;
}

static int
dm_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return dm_set_timeo(timer, ns, TCLR_ONESHOT);
}

static uint64_t
dm_get_time(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    return TICKS_TIMER_INTERVAL(dmt->hw->value);
}

static void
dm_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->intclr = 0x1;
}

static uint32_t
dm_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    return dmt->irq;
}

static pstimer_t timers[NTIMERS];
static dmt_t dmts[NTIMERS];

pstimer_t *
ps_get_timer(enum timer_id id, timer_config_t *config)
{
    pstimer_t *timer;
    dmt_t *dmt;

    if (id >= NTIMERS) {
        return NULL;
    }
    timer = &timers[id];
    dmt = &dmts[id];

    timer->properties.upcounter = false;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;

    timer->data = dmt;
    timer->start = dm_timer_start;
    timer->stop = dm_timer_stop;
    timer->get_time = dm_get_time;
    timer->oneshot_absolute = dm_oneshot_absolute;
    timer->oneshot_relative = dm_oneshot_relative;
    timer->periodic = dm_periodic;
    timer->handle_irq = dm_handle_irq;
    timer->get_nth_irq = dm_get_nth_irq;

    dmt->hw = (struct dmt_map *)config->vaddr;
    dmt->irq = config->irq;

    dm_timer_reset(timer);
    return timer;
}
