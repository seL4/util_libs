/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "timer_priv.h"

/* Driver for the HiSilison hi6220 hikey Dual-timer devices.
 *
 * There are 9 timer devices, each implementing two downcounters for a total
 * of 18 downcounters. These downcounters run at 19.2MHz.
 *
 * The 9 timer devices each have their own physical frame address, but the
 * 2 downcounters for each device reside in the same 4K frame.
 *
 * We have numbered the downcounters from 0-17 as distinct logical devices.
 */

#define TCLR_ONESHOT	BIT(0)
#define TCLR_VALUE_32	BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_STARTTIMER BIT(7)

#define TICKS_PER_SECOND 19200000

/* The dual timers only have up to microsec accuracy.
 * So convert the nanoseconds into microseconds,
 * and program the timer in microseconds internally.
 *
 * If there are actual nanosecond-level offset digits,
 * we just add one more microsecond (such that 1 nanosecond
 * will result in a delay of 1 microsecond, but 999 nanoseconds
 * will also result in a delay of 1 microsecond).
 */
#define TICKS_PER_US    (TICKS_PER_SECOND / 1000000)
#define NS_TO_TICKS(ns) (((ns / NS_IN_US) + ((ns % 1000000) ? 1:0)) * TICKS_PER_US)
#define TICKS_TO_NS(t) ((t * TICKS_PER_US) * NS_IN_US)

#define HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET (0x20)

typedef volatile struct dmt_regs {
	uint32_t load;
	uint32_t value;
	uint32_t control;
	uint32_t intclr;
	uint32_t ris;
	uint32_t mis;
	uint32_t bgload;
} dmt_regs_t;

typedef struct dmt {
    dmt_regs_t *regs;
    int timer_id;
    uint32_t irq;
} dmt_t;


static dmt_regs_t *
hikey_dualtimer_get_secondary_timer(const dmt_t *dual_timer)
{
    dmt_regs_t *ret;

    /* So there are two downcounters for each of the dualtimers.
     * Hitherto we were ignoring the existence of the second one.
     *
     * Now we're using it.
     */
    ret = (dmt_regs_t *)((uintptr_t)dual_timer->regs
          + HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET);

    return ret;
}

static dmt_regs_t *
hikey_dualtimer_get_regs(const pstimer_t *timer)
{
    dmt_t *dmt = (dmt_t*)timer->data;

    /* Even numbered device IDs are at offset 0, odd-numbered device IDs are
     * at offset 0x20 within the same page.
     */
    if (dmt->timer_id / 2 == 0) {
        return dmt->regs;
    } else {
        return hikey_dualtimer_get_secondary_timer(dmt);
    }
}

static void
dm_timer_reset(const pstimer_t *timer)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);

    dmt_regs->control = 0;
}

static int
dm_timer_stop(const pstimer_t *timer)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);

    dmt_regs->control = dmt_regs->control & ~TCLR_STARTTIMER;
    return 0;
}

static int
dm_timer_start(const pstimer_t *timer)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);

    dmt_regs->control = dmt_regs->control | TCLR_STARTTIMER;
    return 0;
}

/* This is exported so it can be reused in virtual_upcounter.c */
int
dm_set_timeo(const pstimer_t *timer, uint64_t ns, int otherFlags)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);
    uint32_t ticks = NS_TO_TICKS(ns);

    dmt_regs->control = 0;

    /* No need to check for ticks == 0, because 0 is a valid value:
     *
     * Hikey Application Processor Function Description, section 2.3, "TIMERN_LOAD":
     *   "The minimum valid value of TIMERN_LOAD is 1. If 0 is written to TIMERN_LOAD, a
     *   timing interrupt is generated immediately."
     *
     * If the user supplies 0 as the argument, they'll just get an IRQ
     * immediately.
     */
    if (otherFlags & TCLR_AUTORELOAD) {
        /* Hikey Application Processor Function Description, section 2.3, "TIMERN_BGLOAD":
         *   "TIMERN_BGLOAD is an initial count value register in periodic mode.
         *
         *   In periodic mode, when the value of TIMERN_BGLOAD is updated, the
         *   value of TIMERN_LOAD is changed to that of TIMERN_BGLOAD. However,
         *   the timer counter does not restart counting. After the counter
         *   decreases to 0, the value of TIMERN_LOAD (that is,
         *   the value of TIMERN_BGLOAD) is reloaded to the counter.
         *   dmt->regs->bgload = ticks;
         *
         * In other words, for periodic mode, load BGLOAD first, then write to
         * LOAD. For oneshot mode, only write to LOAD. For good measure, write 0
         * to BGLOAD.
         */
        dmt_regs->bgload = ticks;
    } else {
        dmt_regs->bgload = 0;
    }
    dmt_regs->load = ticks;

    /* The TIMERN_VALUE register is read-only. */
    dmt_regs->control = TCLR_STARTTIMER| TCLR_VALUE_32
                        | otherFlags;

    return 0;
}

static int
dm_periodic(const pstimer_t *timer, uint64_t ns)
{
    return dm_set_timeo(timer, ns, TCLR_AUTORELOAD | TCLR_INTENABLE );
}

static int
dm_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    return ENOSYS;
}

static int
dm_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return dm_set_timeo(timer, ns, TCLR_ONESHOT | TCLR_INTENABLE );
}

static void
dm_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);
    dmt_regs->intclr = 0x1;
}

static uint32_t
dm_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    dmt_t *dmt = (dmt_t*)timer->data;
    return dmt->irq;
}

static uint64_t
hikey_dualtimer_get_timestamp(const pstimer_t *timer)
{
    dmt_regs_t *dmt_regs = hikey_dualtimer_get_regs(timer);
    uint64_t ret;

    /* Just return the current downcounter count value.
     * 
     * However, convert it into nanoseconds first.
     */
    ret = dmt_regs->value;
    ret = ret / TICKS_PER_US * NS_IN_US;
    return ret;
}

static timer_properties_t dmtimer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,

    .bit_width = 32,
    .irqs = 1
};

typedef struct hikey_dualtimer_descriptor {
    pstimer_t apihandle;
    dmt_t priv;
} hikey_dualtimer_descriptor_t;

static hikey_dualtimer_descriptor_t hikey_dualtimers[NUM_DMTIMERS];

pstimer_t *
hikey_dualtimer_get_timer(int timer_id, timer_config_t *config)
{
    pstimer_t *timer;

    if (timer_id > DMTIMER17) {
        ZF_LOGE("Invalid timer device ID for a hikey dual-timer.");
        return NULL;
    }

    if (config == NULL || config->vaddr == NULL) {
        ZF_LOGE("Vaddr for the mapped dual-timer device register frame is"
                " required.");
        return NULL;
    }

    timer = &hikey_dualtimers[timer_id].apihandle;
    timer->data = &hikey_dualtimers[timer_id].priv;
    timer->properties = dmtimer_props;

    hikey_dualtimers[timer_id].priv.timer_id = timer_id;
    hikey_dualtimers[timer_id].priv.regs = config->vaddr;
    hikey_dualtimers[timer_id].priv.irq = config->irq;

    timer->start = dm_timer_start;
    timer->stop = dm_timer_stop;
    timer->get_time = hikey_dualtimer_get_timestamp;
    timer->oneshot_absolute = dm_oneshot_absolute;
    timer->oneshot_relative = dm_oneshot_relative;
    timer->periodic = dm_periodic;
    timer->handle_irq = dm_handle_irq;
    timer->get_nth_irq = dm_get_nth_irq;

    dm_timer_reset(timer);
    return timer;
}
