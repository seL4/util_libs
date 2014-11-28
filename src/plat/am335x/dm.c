/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>
#include <stdio.h>
#include <assert.h>

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

typedef volatile struct dm {
    uint32_t tidr; // 00h TIDR Identification Register
    uint32_t padding1[3];
    uint32_t cfg; // 10h TIOCP_CFG Timer OCP Configuration Register
    uint32_t padding2[3];
    uint32_t tieoi; // 20h IRQ_EOI Timer IRQ End-Of-Interrupt Register
    uint32_t tisrr; // 24h IRQSTATUS_RAW Timer IRQSTATUS Raw Register
    uint32_t tisr; // 28h IRQSTATUS Timer IRQSTATUS Register
    uint32_t tier; // 2Ch IRQSTATUS_SET Timer IRQENABLE Set Register
    uint32_t ticr; // 30h IRQSTATUS_CLR Timer IRQENABLE Clear Register
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
} dm_t;

static int
dm_stop_timer(const pstimer_t *device)
{
    dm_t *dm = (dm_t *) timer->data;
    /* Disable timer. */
    dm->tier = 0;
    dm->tclr = 0;
    dm->tisr = TISR_OVF_FLAG;
}

static int
dm_start_timer(const pstimer_t *device)
{
    /* Do nothing */
}


static int
dm_periodic(uint64_t ns)
{
    /* Stop time. */
    dm->tclr = 0;

    /* Reset timer. */
    dm->cfg = TIOCP_CFG_SOFTRESET;

    while (dm->cfg & TIOCP_CFG_SOFTRESET);

    /* Enable interrupt on overflow. */
    dm->tier = TIER_OVERFLOWENABLE;

    /* Set the reload value. */
    dm->tldr = ~0UL - TIMER_INTERVAL_TICKS(ns);

    /* Reset the read register. */
    dm->tcrr = ~0UL - TIMER_INTERVAL_TICKS(ns);

    /* Clear pending overflows. */
    dm->tisr = TISR_OVF_FLAG;

    /* Set autoreload and start the timer. */
    dm->tclr = TCLR_AUTORELOAD | TCLR_STARTTIMER;
}

static int
dm_oneshot_absolute(uint64_t ns)
{
    assert(!"Not implemented");
    return ENOSYS;
}

static int
dm_oneshot_relative(uint64_t ns)
{
    assert(!"Not implemented");
    return ENOSYS;
}

static uint64_t
dm_get_time(const pstimer_t *timer)
{
    assert(!"Not implemented");
    return ENOSYS;
}

static void
dm_handle_irq(const pstimer_t *timer)
{
    /* nothing */
}

static uint32_t
dm_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    return DMTIMER2_INTTERRUPT;
}

static pstimer_t singleton_timer;

pstimer_t *
dm_get_timer(void *vaddr)
{
    pstimer_t *timer = &singleton_timer;

    timer->properties.upcounter = false;
    timer->properties.timeouts = 1;
    timer->properties.bitwidth = 32;
    timer->properties.irqs = 1;

    /* data just points to the dm itself for now */
    timer->data = vaddr;
    timer->start = dm_timer_start;
    timer->stop = dm_timer_stop;
    timer->get_time = dm_get_time;
    timer->oneshot_absolute = dm_oneshot_absolute;
    timer->oneshot_relative = dm_oneshot_relative;
    timer->periodic = dm_periodic;
    timer->handle_irq = dm_handle_irq;
    timer->get_nth_irq = dm_get_nth_irq;

    return timer;
}
