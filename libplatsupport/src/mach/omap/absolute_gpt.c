/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <stdbool.h>
#include "gpt.h"
#include "../../stubtimer.h"

/* Absolute timer implementation for gpt.
 *
 * This timer works by reloading 0 on each overflow.
 * For timeouts, the compare register is used.
 *
 * If you try to set a timeout that is too close to the 
 * current time there is a race condition, and irqs will Not
 * come in until the timer overflows.
 */

static uint64_t
abs_gpt_get_time(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    bool overflow;
    uint64_t ticks;

    overflow = !!(gpt->gpt_map->tisr & OVF_IT_FLAG);
    /* high bits */
    ticks = ((uint64_t) (gpt->high_bits + overflow)) << 32llu;
    /* low bits */
    ticks += gpt->gpt_map->tcrr;
    ticks = ticks * (gpt->prescaler + 1);
    
    return gpt_ticks_to_ns(ticks);
}

static int
abs_gpt_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    bool overflow = gpt->gpt_map->tisr & OVF_IT_FLAG;
    uint64_t ticks = gpt_ns_to_ticks(ns) / (gpt->prescaler + 1);
            
    if ((ns >> 32llu) == (gpt->high_bits + !!overflow)) {
        /* Enable match irq */
        gpt->gpt_map->tier |= BIT(MAT_IT_ENA);
        /* set match */
        gpt->gpt_map->tmar = (uint32_t) ticks;
    }
    /* otherwise the overflow irq will trigger first */
    return abs_gpt_get_time(timer) >= ns ? ETIME : 0;
}

static int
abs_gpt_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return abs_gpt_oneshot_absolute(timer, abs_gpt_get_time(timer) + ns);
}

static void
abs_gpt_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    /* turn of match irq */
    gpt->gpt_map->tier &= ~BIT(MAT_IT_ENA);
    gpt_handle_irq(timer, irq);
}

static pstimer_t singleton_timer;
static gpt_t singleton_gpt;

pstimer_t *
abs_gpt_get_timer(gpt_config_t *config)
{
    pstimer_t *timer = &singleton_timer;
    gpt_t *gpt = &singleton_gpt;

    if (!gpt_ok_prescaler(config->prescaler)) {
        return NULL;
    }

    timer->properties = gpt_common_properties();
    timer->properties.absolute_timeouts = true;
    timer->properties.periodic_timeouts = false;
    timer->properties.relative_timeouts = true;

    timer->data = (void *) gpt;
    timer->start = gpt_timer_start;
    timer->stop = gpt_timer_stop;
    timer->get_time = abs_gpt_get_time;
    timer->oneshot_absolute = abs_gpt_oneshot_absolute;
    timer->oneshot_relative = abs_gpt_oneshot_relative;
    timer->periodic = stub_timer_timeout;
    timer->handle_irq = abs_gpt_handle_irq;
    timer->get_nth_irq = gpt_get_nth_irq;

    gpt->id = config->id;
    gpt->gpt_map = (volatile struct gpt_map*) config->vaddr;

    /* Enable interrupt on overflow. */
    gpt->gpt_map->tier |= BIT(OVF_IT_ENA);

    /* Set the reload value. */
    gpt->gpt_map->tldr = 0u;

    /* Reset the read register. */
    gpt->gpt_map->tcrr = 0u;

    /* Clear pending irqs. */
    gpt->gpt_map->tisr |= BIT(OVF_IT_FLAG | MAT_IT_FLAG | TCAR_IT_FLAG);

    gpt->gpt_map->tclr |= (BIT(CE) | BIT(AR));
    
    gpt_init(gpt);
    
    return timer;
}

