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
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "../../stubtimer.h"

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

/* 
 * This timer will only work if the kernel has configured
 * CNTPCT to be read from user-level. This is done by writing 1 to CNTKCTL
 */

#define MRRC(cpreg, v)  asm volatile("mrrc  " cpreg :  "=r"(v))
#define CNTPCT " p15, 0, %Q0, %R0, c14"

static uint64_t
generic_timer_get_time(const pstimer_t *timer)
{
    uint64_t time;
    MRRC(CNTPCT, time);

    /* convert to ns */
    return time / PCT_NS_PER_US * NS_IN_US;
}

static pstimer_t singleton_timer;

pstimer_t *
generic_timer_get_timer(void)
{
    pstimer_t *timer = &singleton_timer;
    
    stub_timer_get_timer(timer);

    timer->properties.upcounter = true;
    timer->properties.timeouts = false;
    timer->properties.bit_width = 64;
    timer->properties.irqs = 0;

    timer->get_time = generic_timer_get_time;

    return timer;
}

#endif /* CONFIG_ARCH_ARM_V7A */
#endif /* CONFIG_ARM_CORTEX_A15 */
