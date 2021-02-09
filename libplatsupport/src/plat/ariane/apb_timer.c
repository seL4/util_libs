/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/apb_timer.h>

/* Largest timeout that can be programmed */
#define MAX_TIMEOUT_NS (BIT(31) * (NS_IN_S / APB_TIMER_INPUT_FREQ))

int apb_timer_start(apb_timer_t *apb_timer)
{
    apb_timer->apb_timer_map->ctrl |= APB_TIMER_CTRL_ENABLE;
    return 0;
}

int apb_timer_stop(apb_timer_t *apb_timer)
{
    /* Disable timer. */
    apb_timer->apb_timer_map->cmp = CMP_MASK;
    apb_timer->apb_timer_map->ctrl &= ~APB_TIMER_CTRL_ENABLE;
    apb_timer->apb_timer_map->time = 0;

    return 0;
}

int apb_timer_set_timeout(apb_timer_t *apb_timer, uint64_t ns)
{
    if (ns > MAX_TIMEOUT_NS) {
        ZF_LOGE("Cannot program a timeout larget than %ld ns", MAX_TIMEOUT_NS);
        return -1;
    }

    apb_timer->apb_timer_map->cmp = ns / (NS_IN_S / APB_TIMER_INPUT_FREQ);
    apb_timer_start(apb_timer);

    return 0;
}

uint64_t apb_timer_get_time(apb_timer_t *apb_timer)
{
    uint64_t ticks = apb_timer->apb_timer_map->time + apb_timer->time_h;
    return ticks * (NS_IN_S / APB_TIMER_INPUT_FREQ);
}

int apb_timer_init(apb_timer_t *apb_timer, apb_timer_config_t config)
{
    apb_timer->apb_timer_map = (volatile struct apb_timer_map *) config.vaddr;
    apb_timer->time_h = 0;
    return 0;
}
