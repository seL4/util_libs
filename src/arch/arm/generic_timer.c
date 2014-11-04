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

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

/* 
 * This timer will only work if the kernel has configured
 * CNTPCT to be read from user-level. This is done by writing 1 to CNTKCTL
 */

#define MRRC(cpreg, v)  asm volatile("mrrc  " cpreg :  "=r"(v))
#define CNTPCT " p15, 0, %Q0, %R0, c14"

static int
start(const pstimer_t *timer)
{
    return 0;
}

static int
stop(const pstimer_t *timer)
{
    return 0;
}

static int
oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not supported");
    return ENOSYS;
}

static int
periodic(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not supported");
    return ENOSYS;
}

static int
oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not supported");
    return ENOSYS;
}

static void
handle_irq(const pstimer_t *timer, uint32_t irq)
{
}


static uint64_t
get_time(const pstimer_t *timer)
{
    uint64_t time;
    MRRC(CNTPCT, time);

    /* convert to ns */
    return time / PCT_NS_PER_US * NS_IN_US;
}

static uint32_t
get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    assert(!"Not supported");
    return ENOSYS;
}

static pstimer_t singleton_timer;

pstimer_t *
generic_timer_get_timer(void)
{
    pstimer_t *timer = &singleton_timer;

    timer->properties.upcounter = true;
    timer->properties.timeouts = false;
    timer->properties.bit_width = 64;
    timer->properties.irqs = 0;

    timer->data = NULL;
    timer->start = start;
    timer->stop = stop;
    timer->get_time = get_time;
    timer->oneshot_absolute = oneshot_absolute;
    timer->oneshot_relative = oneshot_relative;
    timer->periodic = periodic;
    timer->handle_irq = handle_irq;
    timer->get_nth_irq = get_nth_irq;

    return timer;
}

#endif /* CONFIG_ARCH_ARM_V7A */
#endif /* CONFIG_ARM_CORTEX_A15 */
