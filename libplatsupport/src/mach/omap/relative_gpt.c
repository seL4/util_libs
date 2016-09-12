/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "gpt.h"
#include "../../stubtimer.h"

/* Relative timeout driver for the gpt.
 *
 * This driver sets up the gpt for relative timeouts (oneshot or periodic) only.
 *
 * It works by setting the timer to interrupt on overflow and reloading the timer
 * with (0xFFFFFFFF - relative timeout).
 */

int
configure_timeout(gpt_t *gpt, uint64_t ns, uint32_t reload)
{
    uint64_t ticks = gpt_ns_to_ticks(ns) / (gpt->prescaler + 1);

    if (ticks >= UINT32_MAX) {
        /* too big for this timer implementation */
        ZF_LOGE("Timeout too big for timer, max %u, got %llu\n", UINT32_MAX - 1, ticks);
        return ETIME;
    }

    /* invert ticks - it's an upcounter and will interrupt on overflow */
    ticks = UINT32_MAX - ticks;

    gpt_init(gpt);

    /* Clear pending overflows. */
    gpt->gpt_map->tisr |= BIT(OVF_IT_FLAG);

    /* Set the reload value. */
    gpt->gpt_map->tldr = (uint32_t) ticks;

    /* Reset the read register. */
    gpt->gpt_map->tcrr = (uint32_t) ticks;

    /* Enable interrupt on overflow. */
    gpt->gpt_map->tier |= BIT(OVF_IT_ENA);

    assert(!(gpt->gpt_map->tisr & BIT(OVF_IT_FLAG)));
    /* Set autoreload and start the timer. */
    gpt->gpt_map->tclr |= (reload | BIT(ST));

    /* success */
    return 0;
}

static int
rel_gpt_periodic(const pstimer_t *timer, uint64_t ns)
{
    return configure_timeout((gpt_t *) timer->data, ns, BIT(AR));
}

static int
rel_gpt_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return configure_timeout((gpt_t *) timer->data, ns, 0u);
}

static uint64_t
rel_gpt_get_time(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    return gpt_ticks_to_ns(gpt->gpt_map->tcrr) * (gpt->prescaler + 1);
}

static pstimer_t singleton_timer;
static gpt_t singleton_gpt;

pstimer_t *
rel_gpt_get_timer(gpt_config_t *config)
{
    pstimer_t *timer = &singleton_timer;
    gpt_t *gpt = &singleton_gpt;

    if (!gpt_ok_prescaler(config->prescaler)) {
        return NULL;
    }

    timer->properties = gpt_common_properties();
    timer->properties.timeouts = true;
    timer->properties.absolute_timeouts = false;
    timer->properties.relative_timeouts = true;
    timer->properties.periodic_timeouts = true;

    timer->data = (void *) gpt;
    timer->start = gpt_timer_start;
    timer->stop = gpt_timer_stop;
    timer->get_time = rel_gpt_get_time;
    timer->oneshot_absolute = stub_timer_timeout;
    timer->oneshot_relative = rel_gpt_oneshot_relative;
    timer->periodic = rel_gpt_periodic;
    timer->handle_irq = gpt_handle_irq;
    timer->get_nth_irq = gpt_get_nth_irq;

    gpt->id = config->id;
    gpt->gpt_map = (volatile struct gpt_map*) config->vaddr;
    gpt->prescaler = config->prescaler;

    gpt_init(gpt);

    return timer;
}
