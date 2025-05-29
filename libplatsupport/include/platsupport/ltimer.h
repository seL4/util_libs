/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/io.h>
#include <platsupport/pmem.h>
#include <platsupport/irq.h>
#include <platsupport/gen_config.h>

/**
 * This file provides the interface for an OS independant consisent timer interface.
 *
 * Implementations of this interface vary per platform - for some platforms,
 * a single timer driver will back the implementation, for others, there may be many.
 */

typedef enum {
    TIMEOUT_PERIODIC,
    TIMEOUT_ABSOLUTE,
    TIMEOUT_RELATIVE
} timeout_type_t;

typedef enum {
    LTIMER_TIMEOUT_EVENT,
    LTIMER_OVERFLOW_EVENT
} ltimer_event_t;

/*
 * Type signature of the callback function that can be accepted by the ltimer.
 * The callbacks are invoked when an event occurs. These events are described
 * by the ltimer_event_t type.
 *
 * The callbacks are expected to be reentrant with regards to the ltimer state.
 * More specifically, the ltimer interface cannot guarantee that the internal
 * state will be race-safe if two different callbacks invoke the ltimer API
 * functions concurrently.
 */
typedef void (*ltimer_callback_fn_t)(void *token, ltimer_event_t event);

/* logical timers are the interface used by the timer manager to multiplex
 * timer requests from clients - only one timeout can be registered at a time.
 * logical timers may be backed by several timer drivers to implement the
 * functionality
 */
typedef struct ltimer {
    /*
     * Get the number of irqs this ltimer requires to be handled to function.
     *
     * @param data       for the logical timer to use
     * @return           the number of irqs this timer needs.
     */
    size_t (*get_num_irqs)(void *data);

    /*
     * Get the nth irq number.
     *
     * @param data       for the logical timer to use
     * @param n          index of the irq, < get_num_irqs
     * @param[out] irq   variable to read the irq number into
     * @return           0 on success, errno on error.
     */
    int (*get_nth_irq)(void *data, size_t n, ps_irq_t *irq);

    /* Get the number of phyiscal memory regions needed by this ltimer
     *
     * @param data       for the logical timer to use
     * @return           the number of pmem regions this timer needs.
     */
    size_t (*get_num_pmems)(void *data);

    /*
     * Populate a region structure with details of the nth pmem region this timer requires.
     *
     * @param data       for the logical timer to use
     * @param n          index of the pmem_region, < get_num_pmems
     * @return           0 on success, errno on error.
     */
    int (*get_nth_pmem)(void *data, size_t n, pmem_region_t *region);

    /*
     * Read the current time in nanoseconds. Precision depends on the implementation, but
     * the value is guaranteed to be monotonically increasing and at least millisecond accurate.
     *
     * @param data     for the logical timer to use
     * @param[out] time variable to read the time value into
     * @return          0 on success, errno on error.
     */
    int (*get_time)(void *data, uint64_t *time);

    /*
     * Get the precision of this time returned by get_time. i.e if the timer is
     * millisecond precise return NS_IN_US.
     *
     * @param data     for the logical timer to use
     * @param[out]     resolution variable to read the resoltion value into
     * @return         0 on success, errno on error.
     */
    int (*get_resolution)(void *data, uint64_t *resolution);

    /*
     * Set an irq to come in at a specific time.
     *
     * IRQs may come in earlier than requested due to implementation details.
     *
     * @param data     for the logical timer to use
     * @param ns       ns value (depends on timer type)
     * @param type     type of timeout
     * @return         0 on success, errno on error.
     */
    int (*set_timeout)(void *data, uint64_t ns, timeout_type_t type);

    /*
     * Reset the timer into a state similar to what it was when first initialized. This
     * should cancel any outstanding timeouts etc. It may or may not cause the monotonic
     * upcounter to change (by resetting to 0 or otherwise)
     *
     * @param data     for the logical timer to use
     * @return         0 on success, errno on error.
     */
    int (*reset)(void *data);

    /* Destroy an ltimer, freeing any resources and turning off devices */
    void (*destroy)(void *data);

    /* data for the implementation to use */
    void *data;
} ltimer_t;

/* Logical timer helper functions */
static inline int ltimer_get_resolution(ltimer_t *timer, uint64_t *resolution)
{
    if (!timer) {
        ZF_LOGE("Logical timer invalid!");
        return EINVAL;
    }

    if (!resolution) {
        ZF_LOGE("time argument cannot be NULL");
        return EINVAL;
    }

    if (timer->get_resolution == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    return timer->get_resolution(timer->data, resolution);
}

static inline int ltimer_set_timeout(ltimer_t *timer, uint64_t nanoseconds, timeout_type_t type)
{
    if (!timer) {
        ZF_LOGE("Logical timer invalid!");
        return EINVAL;
    }

    if (timer->set_timeout == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    switch (type) {
    case TIMEOUT_ABSOLUTE:
    case TIMEOUT_PERIODIC:
    case TIMEOUT_RELATIVE:
        break;
    default:
        ZF_LOGE("Invalid timer type");
        return EINVAL;
    }

    return timer->set_timeout(timer->data, nanoseconds, type);
}

static inline size_t ltimer_get_num_irqs(ltimer_t *timer)
{
    if (timer->get_num_irqs == NULL) {
        /* assume no irqs for this timer */
        return 0;
    }

    return timer->get_num_irqs(timer->data);
}

static inline int ltimer_get_nth_irq(ltimer_t *timer, size_t n, ps_irq_t *irq)
{
    if (!timer || !irq) {
        ZF_LOGE("Arguments cannot be null");
        return EINVAL;
    }

    if (!timer->get_nth_irq || !timer->get_num_irqs) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    size_t nirqs = ltimer_get_num_irqs(timer);
    if (n >= nirqs) {
        ZF_LOGD("n invalid");
        return EINVAL;
    }

    return timer->get_nth_irq(timer->data, n, irq);
}

static inline size_t ltimer_get_num_pmems(ltimer_t *timer)
{
    if (timer->get_num_pmems == NULL) {
        /* assume no physical memory required for this ltimer */
        return 0;
    }

    return timer->get_num_pmems(timer->data);
}

static inline int ltimer_get_nth_pmem(ltimer_t *timer, size_t n, pmem_region_t *pmem)
{
    if (!timer || !pmem) {
        ZF_LOGE("Arguments cannot be null");
        return EINVAL;
    }

    if (!timer->get_nth_pmem || !timer->get_num_pmems) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    size_t npmems = ltimer_get_num_pmems(timer);
    if (n >= npmems) {
        ZF_LOGD("n invalid");
        return EINVAL;
    }

    return timer->get_nth_pmem(timer->data, n, pmem);
}

static inline int ltimer_get_time(ltimer_t *timer, uint64_t *time)
{
    if (!timer) {
        ZF_LOGE("Logical timer invalid!");
        return EINVAL;
    }

    if (!time) {
        ZF_LOGE("time argument cannot be NULL");
        return EINVAL;
    }

    if (timer->get_time == NULL) {
        ZF_LOGE("get_time not implemented");
        return ENOSYS;
    }

    return timer->get_time(timer->data, time);
}

static inline int ltimer_reset(ltimer_t *timer)
{
    if (!timer) {
        ZF_LOGE("Logical timer invalid!");
        return EINVAL;
    }

    if (timer->reset == NULL) {
        ZF_LOGE("not implemented");
        return ENOSYS;
    }

    return timer->reset(timer->data);
}

static inline void ltimer_destroy(ltimer_t *timer)
{
    if (!timer || !timer->destroy) {
        ZF_LOGW("nothing to destroy");
        return;
    }

    return timer->destroy(timer->data);
}

/* Spinning delay functions */
static inline void ltimer_ns_delay(ltimer_t *timer, uint64_t nanoseconds)
{
    uint64_t start, end;

    int error = ltimer_get_time(timer, &start);
    /* spin */
    for (int i = 0; !error; i++) {
        error = ltimer_get_time(timer, &end);
        if (end - start >= nanoseconds) {
            break;
        } else if (i % 1000 == 0 && start == end) {
            ZF_LOGD("Time doesn't appear to be changing");
        }
    }
}

static inline void ltimer_s_delay(ltimer_t *timer, uint64_t seconds)
{
    ltimer_ns_delay(timer, seconds * NS_IN_S);
}

static inline void ltimer_ms_delay(ltimer_t *timer, uint64_t milliseconds)
{
    ltimer_ns_delay(timer, milliseconds * NS_IN_MS);
}

static inline void ltimer_us_delay(ltimer_t *timer, uint64_t microseconds)
{
    ltimer_ns_delay(timer, microseconds * NS_IN_US);
}


/*
 * default init function -> platforms may provide multiple ltimers, but each
 * must have a default
 *
 * The callback functions will be invoked every single time an event described
 * by the 'ltimer_event_t' type occurs. A reminder that care should be taken
 * with calling the ltimer functions inside the callback, the ltimer interface
 * functions are not reentrant.
 */
#ifndef CONFIG_LIB_PLAT_SUPPORT_HAVE_TIMER
__attribute__((error("no ltimer support for this platform")))
#endif
int ltimer_default_init(ltimer_t *timer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token);

/* initialise the subset of functions required to get
 * the resources this ltimer needs without initialising the actual timer
 * drivers*/
#ifndef CONFIG_LIB_PLAT_SUPPORT_HAVE_TIMER
__attribute__((error("no ltimer support for this platform")))
#endif
int ltimer_default_describe(ltimer_t *timer, ps_io_ops_t ops);
