/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __STUB_TIMER_H
#define __STUB_TIMER_H

#include <platsupport/timer.h>

#include <stdio.h>
#include <assert.h>

static int
stub_timer_start(const pstimer_t *device)
{
    return 0;
}

static int
stub_timer_stop(const pstimer_t *device)
{
    return 0;
}

static uint64_t
stub_timer_get_time(const pstimer_t *device)
{
    return 0llu;
}

static int
stub_timer_timeout(const pstimer_t *device, uint64_t ns)
{
    assert(!"Not implemented");
    return ENOSYS;
}

static void
stub_timer_handle_irq(const pstimer_t *device, uint32_t irq)
{
}

static uint32_t
stub_timer_get_nth_irq(const pstimer_t *device, uint32_t n)
{
    return 0;
}

static inline void
stub_timer_get_timer(pstimer_t *timer)
{
    timer->start = stub_timer_start;
    timer->stop = stub_timer_stop;
    timer->get_time = stub_timer_get_time;
    timer->oneshot_absolute = stub_timer_timeout;
    timer->oneshot_relative = stub_timer_timeout;
    timer->periodic = stub_timer_timeout;
    timer->handle_irq = stub_timer_handle_irq;
    timer->get_nth_irq = stub_timer_get_nth_irq;
    timer->data = NULL;
}

#endif /* STUB_TIMER_H */
