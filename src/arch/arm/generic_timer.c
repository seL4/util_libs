/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "../../stubtimer.h"

#ifdef CONFIG_EXPORT_PCNT_USER

/*
 * This timer will only work if the kernel has configured
 * CNTPCT to be read from user-level. This is done by writing 1 to CNTKCTL.
 * If CONFIG_DANGEROUS_CODE_INJECTION is available, we'll try that,
 * otherwise we will use a default frequency.
 *
 * If all else fails the timer will fail to initialise.
 *
 */
#define MCR(cpreg, v)                               \
    do {                                            \
        uint32_t _v = v;                            \
        asm volatile("mcr  " cpreg :: "r" (_v));    \
    }while(0)
#define MRRC(cpreg, v)  asm volatile("mrrc  " cpreg :  "=r"(v))
#define MRC(cpreg, v)  asm volatile("mrc  " cpreg :  "=r"(v))

#define CNTFRQ     " p15, 0,  %0, c14,  c0, 0" /* 32-bit RW Counter Frequency register */
#define CNTPCT     " p15, 0, %Q0, %R0, c14" /* 64-bit RO Physical Count register */
#define CNTKCTL    " p15, 0,  %0, c14,  c1, 0" /* 32-bit RW Timer PL1 Control register */

static uint64_t
generic_timer_get_time(const pstimer_t *timer)
{
    uint64_t time;
    uint32_t freq = (uint32_t) timer->data;

    MRRC(CNTPCT, time);

    /* convert to ns */
    return time / (uint64_t) freq * NS_IN_US;
}

static pstimer_t singleton_timer;

pstimer_t *
generic_timer_get_timer(void)
{
    pstimer_t *timer = &singleton_timer;
    uint32_t freq = 0;

    /* try to read the frequency */
    MRC(CNTFRQ, freq);

#ifdef PCT_TICKS_PER_US
    if (freq == 0) {
        freq = PCT_TICKS_PER_US;
    }
#endif

    if (freq == 0) {
        /* fail init, we don't know what the frequency of the timer is */
        return NULL;
    }

    stub_timer_get_timer(timer);

    timer->data = (void *) freq;
    timer->properties.upcounter = true;
    timer->properties.timeouts = false;
    timer->properties.bit_width = 64;
    timer->properties.irqs = 0;

    timer->get_time = generic_timer_get_time;

    return timer;
}

#endif /* CONFIG_EXPORT_PCNT_USER */
