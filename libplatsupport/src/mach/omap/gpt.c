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

int
gpt_timer_start(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    gpt->gpt_map->tclr |= BIT(ST);

    return 0;
}

int
gpt_timer_stop(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    /* Disable timer. */
    gpt->gpt_map->tclr = 0;

    return 0;
}

void
gpt_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    if (gpt->gpt_map->tisr & BIT(OVF_IT_FLAG)) {
        gpt->high_bits++;
    }

    /* ack any possible irqs */
    gpt->gpt_map->tisr = (BIT(OVF_IT_FLAG) | BIT(MAT_IT_FLAG) | BIT(TCAR_IT_FLAG));
}

uint32_t
gpt_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    return omap_get_gpt_irq(gpt->id);
}

bool
gpt_ok_prescaler(uint32_t prescaler)
{
    if (prescaler > 7) {
        ZF_LOGE("Prescaler value set too large for device, value: %d, max 7", prescaler);
        return false;
    }

    return true;
}

uint64_t 
gpt_ticks_to_ns(uint64_t ticks)
{
    return (ticks / CLK_MHZ) * NS_IN_US;
}

uint64_t 
gpt_ns_to_ticks(uint64_t ns)
{
    return ns / NS_IN_US * CLK_MHZ;
}

timer_properties_t
gpt_common_properties(void) {
    timer_properties_t properties = {
        .upcounter = true,
        .bit_width = 32,
        .irqs = 1
    };
    return properties;
}

void 
gpt_init(gpt_t *gpt)
{
    /* Disable GPT. */
    gpt->gpt_map->tclr = 0;

    /* Perform a soft reset */
    gpt->gpt_map->cfg = BIT(SOFTRESET);

    while (!gpt->gpt_map->tistat); /* Wait for timer to reset */

    /* set prescaler */
    if (gpt->prescaler > 0) {
        gpt->gpt_map->tclr = (gpt->prescaler << PTV); /* Set the prescaler */
        gpt->gpt_map->tclr |= BIT(PRE); /* Enable the prescaler */
    }
}

