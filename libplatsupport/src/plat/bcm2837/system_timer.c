/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>
#include "../../arch/arm/clock.h"
#include <platsupport/timer.h>
#include <platsupport/plat/system_timer.h>

/*
 * The system timer on the BCM283[5-7] is fairly simple in its nature.
 * It has a 64-bit free-running counter and 4 compare registers. When
 * the lower 32 bits of the free-running counter match one of the
 * compare registers, an associated IRQ is generated and an associated
 * control bit is set in the control register.
 */

int system_timer_init(system_timer_t *timer, system_timer_config_t config)
{
    if (timer == NULL || config.vaddr == NULL) {
        return EINVAL;
    }

    timer->regs = config.vaddr;

    return 0;
}

uint64_t system_timer_get_time(system_timer_t *timer)
{
    if (timer == NULL) {
        return EINVAL;
    }

    uint64_t initial_high = timer->regs->counter_high;
    uint64_t low = timer->regs->counter_low;
    uint64_t high = timer->regs->counter_high;
    if (high != initial_high) {
        /* get low again if high has ticked over. */
        low = timer->regs->counter_low;
    }

    uint64_t ticks = (high << 32) | low;
    uint64_t time = ticks * SYSTEM_TIMER_NS_PER_TICK;

    return time;
}

int system_timer_set_timeout(system_timer_t *timer, uint64_t ns)
{
    if (timer == NULL) {
        return EINVAL;
    }

    /* Can only set a timeout within the next 2^32 microseconds. */
    uint64_t time = system_timer_get_time(timer);
    uint64_t ticks = time / SYSTEM_TIMER_NS_PER_TICK;
    uint64_t timeout_ticks = ns / SYSTEM_TIMER_NS_PER_TICK;
    if (timeout_ticks < ticks) {
        ZF_LOGE("Timeout in the past\n");
        return ETIME;
    } else if ((timeout_ticks - ticks) > UINT32_MAX) {
        ZF_LOGE("Timeout too far in the future\n");
        return ETIME;
    }

    /* Clear any existing interrupt. */
    timer->regs->ctrl = BIT(SYSTEM_TIMER_MATCH);

    uint32_t timeout = timeout_ticks & MASK(32);
    timer->regs->compare[SYSTEM_TIMER_MATCH] = timeout;

    time = system_timer_get_time(timer);
    if (time >= ns && !(timer->regs->ctrl & BIT(SYSTEM_TIMER_MATCH))) {
        timer->regs->ctrl = BIT(SYSTEM_TIMER_MATCH);
        ZF_LOGE("Timeout missed\n");
        return ETIME;
    }

    return 0;
}

int system_timer_handle_irq(system_timer_t *timer)
{
    if (timer == NULL) {
        return EINVAL;
    }

    timer->regs->ctrl = BIT(SYSTEM_TIMER_MATCH);

    return 0;
}

int system_timer_reset(system_timer_t *timer)
{
    if (timer == NULL) {
        return EINVAL;
    }

    /* Just clear the one timer that is used. */
    timer->regs->ctrl = BIT(SYSTEM_TIMER_MATCH);

    return 0;
}
