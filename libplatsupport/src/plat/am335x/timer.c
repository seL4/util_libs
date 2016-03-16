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

#define TIOCP_CFG_SOFTRESET BIT(0)

#define TIER_MATCHENABLE BIT(0)
#define TIER_OVERFLOWENABLE BIT(1)
#define TIER_COMPAREENABLE BIT(2)

#define TCLR_AUTORELOAD BIT(1)
#define TCLR_COMPAREENABLE BIT(6)
#define TCLR_STARTTIMER BIT(0)

#define TISR_OVF_FLAG (BIT(0) | BIT(1) | BIT(2))

#define TICKS_PER_SECOND 24000000  /* TODO: Pin this frequency down without relying on u-boot. */
#define TIMER_INTERVAL_TICKS(ns) ((uint32_t)(1ULL * (ns) * TICKS_PER_SECOND / 1000 / 1000 / 1000))
#define TICKS_TIMER_INTERVAL(x)  (((uint64_t)x * 1000 * 1000 * 1000) / TICKS_PER_SECOND)

struct dmt_map {
    uint32_t tidr; // 00h TIDR Identification Register
    uint32_t padding1[3];
    uint32_t cfg; // 10h TIOCP_CFG Timer OCP Configuration Register
    uint32_t padding2[3];
    uint32_t tieoi; // 20h IRQ_EOI Timer IRQ End-Of-Interrupt Register
    uint32_t tisrr; // 24h IRQSTATUS_RAW Timer IRQSTATUS Raw Register
    uint32_t tisr; // 28h IRQSTATUS Timer IRQSTATUS Register
    uint32_t tier; // 2Ch IRQENABLE_SET Timer IRQENABLE Set Register
    uint32_t ticr; // 30h IRQENABLE_CLR Timer IRQENABLE Clear Register
    uint32_t twer; // 34h IRQWAKEEN Timer IRQ Wakeup Enable Register
    uint32_t tclr; // 38h TCLR Timer Control Register
    uint32_t tcrr; // 3Ch TCRR Timer Counter Register
    uint32_t tldr; // 40h TLDR Timer Load Register
    uint32_t ttgr; // 44h TTGR Timer Trigger Register
    uint32_t twps; // 48h TWPS Timer Write Posted Status Register
    uint32_t tmar; // 4Ch TMAR Timer Match Register
    uint32_t tcar1; // 50h TCAR1 Timer Capture Register
    uint32_t tsicr; // 54h TSICR Timer Synchronous Interface Control Register
    uint32_t tcar2; // 58h TCAR2 Timer Capture Register
};

typedef struct dmt {
    volatile struct dmt_map *hw;
    uint32_t irq;
} dmt_t;

static void
dm_timer_reset(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->tclr = 0;          /* stop */
    dmt->hw->cfg = TIOCP_CFG_SOFTRESET;
    while (dmt->hw->cfg & TIOCP_CFG_SOFTRESET);
    dmt->hw->tier = TIER_OVERFLOWENABLE;
}

static int
dm_timer_stop(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->tclr = dmt->hw->tclr & ~TCLR_STARTTIMER;
    return 0;
}

static int
dm_timer_start(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->tclr = dmt->hw->tclr | TCLR_STARTTIMER;
    return 0;
}

static int
dm_set_timeo(const pstimer_t *timer, uint64_t ns, int tclrFlags)
{
    dmt_t *dmt = (dmt_t*) timer->data;

    dmt->hw->tclr = 0;      /* stop */

    /* XXX handle prescaler */
    /* XXX handle invalid arguments with an error return */

    uint32_t ticks = TIMER_INTERVAL_TICKS(ns);
    if (ticks < 2) {
        return EINVAL;
    }
    //printf("timer %lld ns = %x ticks (cntr %x)\n", ns, ticks, (uint32_t)(~0UL - ticks));
    dmt->hw->tldr = ~0UL - ticks;   /* reload value */
    dmt->hw->tcrr = ~0UL - ticks;   /* counter */
    dmt->hw->tisr = TISR_OVF_FLAG;  /* ack any pending overflows */
    dmt->hw->tclr = TCLR_STARTTIMER | tclrFlags;
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
    return ENOSYS;      /* not available for downcounters */
}

static int
dm_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return dm_set_timeo(timer, ns, 0);
}

// XXX write test case to verify this...
static uint64_t
dm_get_time(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    uint32_t ticks = ~0UL - dmt->hw->tcrr;
    //printf("get time has %x ticks left or %lld ns\n", ticks, TICKS_TIMER_INTERVAL(ticks));
    return TICKS_TIMER_INTERVAL(ticks);
}

static void
dm_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    dmt_t *dmt = (dmt_t*) timer->data;
    dmt->hw->tisr = TISR_OVF_FLAG;  /* ack any pending overflows */
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
    timer->properties.relative_timeouts = true;
    timer->properties.periodic_timeouts = true;
    timer->properties.absolute_timeouts = false;
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
    // XXX support config->prescaler.

    dm_timer_reset(timer);
    return timer;
}
