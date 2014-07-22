/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_TIMER_H__
#define __PLATSUPPORT_TIMER_H__

#include <stdint.h>
#include <errno.h>

typedef struct pstimer pstimer_t;

/* Properties of a timer */
typedef struct {
        /* Timers are up counters or down counters.
         *
         * Up counters count up from 0, down counters count down from
         * a set value (set when a timeout is set up).
         *
         * Up counters roll over, down counters stop at 0 (oneshot) or reload the timer value (periodic).
         *
         * If the timer is not an uptimer it does not support absolute timeouts.
         */
        uint32_t upcounter:1;

        /* True if this timer supports setting timeouts at all */
        uint32_t timeouts:1;

        /* when does this timer roll over? This will be 0 for down-counters (max valueue 64) */
        uint32_t bit_width:7;

        /* Number of irqs this timer issues */
        uint32_t irqs;
} timer_properties_t;


struct pstimer {
    /* Properties of this timer */
    timer_properties_t properties;
    /*
     * Start the timer. Timer must be initialised.
     *
     * @return 0 on success.
     */
    int  (*start)(const pstimer_t* device);

    /* Stop the timer. init does not need to be called again.
     *
     * @return 0 on success.
     */
    int  (*stop)(const pstimer_t* device);

    /**
     * Get the current time since this timer last overflowed or was reset.
     *
     */
    uint64_t (*get_time)(const pstimer_t* device);

    /**
     * Set an irq to come in at ns.
     * This operation is only supported by count up timers.
     *
     * @param ns the absolute time in nanoseconds to fire an irq.
     * @return 0 on success.
     *         ENOSYS if unsupported,
     *         ETIME if that time has already passed,
     *         EINVAL if ns is beyond what the timer supports (can't count that high).
     */
    int (*oneshot_absolute)(const pstimer_t* device, uint64_t ns);

    /* Set an irq for current time + ns.
     *
     * @param ns nanoseconds in the future to fire an irq.
     * @return 0 on success.
     *         ENOSYS if unsupported.
     *         ETIME if ns is too small and a race occurs.
     *         EINVAL if ns is too big for the timer.
     */
    int (*oneshot_relative)(const pstimer_t* device, uint64_t ns);

    /* Set an irq to keep recurring every time ns passes.
     *
     * If ns is too small and causes races on any but the first irq, periodic
     * mode will be disabled.
     *
     * @param ns amout of time in nanoseconds to fire a recurring irq.
     * @return 0 on success.
     *         ENOSYS if unsupported.
     *         ETIME if ns is too small and a race occurs.
     *         EINVAL if ns is too big for the timer.
     */
    int (*periodic)(const pstimer_t* device, uint64_t ns);

    /*
     * Handle an irq.
     * Call this after every irq comes in.
     *
     * @param irq the irq number that fired.
     */
    void (*handle_irq)(const pstimer_t* device, uint32_t irq);

    /*
     * @return the nth irq number that this timer generates.
     */
    uint32_t (*get_nth_irq)(const pstimer_t *device, uint32_t n);

    /* data for the timer impl */
    void* data;
};

/* Basic wrappers for code neatness */
static inline int
timer_start(pstimer_t* device)
{
    return device->start(device);
}

static inline int
timer_stop(pstimer_t* device)
{
    return device->stop(device);
}

static inline uint64_t
timer_get_time(pstimer_t* device)
{

    return device->get_time(device);
}

static inline int
timer_oneshot_absolute(pstimer_t* device, uint64_t ns)
{
    return device->oneshot_absolute(device, ns);
}

static inline int
timer_oneshot_relative(pstimer_t* device, uint64_t ns)
{
    return device->oneshot_relative(device, ns);
}


static inline int
timer_periodic(pstimer_t* device, uint64_t ns)
{
    return device->periodic(device, ns);
}

static inline void
timer_handle_irq(pstimer_t* device, uint32_t irq)
{
    device->handle_irq(device, irq);
}

static inline uint32_t
timer_get_nth_irq(const pstimer_t *device, uint32_t n)
{
    return device->get_nth_irq(device, n);
}



#endif /* __PLATSUPPORT_TIMER_H__ */
