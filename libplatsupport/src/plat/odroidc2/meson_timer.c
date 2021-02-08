/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>
#include <platsupport/timer.h>
#include <platsupport/plat/meson_timer.h>

int meson_init(meson_timer_t *timer, meson_timer_config_t config)
{
    if (timer == NULL || config.vaddr == NULL) {
        return EINVAL;
    }

    timer->regs = (void *)((uintptr_t) config.vaddr + (TIMER_BASE + TIMER_REG_START * 4 - TIMER_MAP_BASE));

    timer->regs->mux = TIMER_A_EN | (TIMESTAMP_TIMEBASE_1_US << TIMER_E_INPUT_CLK) |
                       (TIMEOUT_TIMEBASE_1_MS << TIMER_A_INPUT_CLK);

    timer->regs->timer_e = 0;

    return 0;
}

uint64_t meson_get_time(meson_timer_t *timer)
{
    uint64_t initial_high = timer->regs->timer_e_hi;
    uint64_t low = timer->regs->timer_e;
    uint64_t high = timer->regs->timer_e_hi;
    if (high != initial_high) {
        low = timer->regs->timer_e;
    }

    uint64_t ticks = (high << 32) | low;
    uint64_t time = ticks * NS_IN_US;
    return time;
}

void meson_set_timeout(meson_timer_t *timer, uint16_t timeout, bool periodic)
{
    if (periodic) {
        timer->regs->mux |= TIMER_A_MODE;
    } else {
        timer->regs->mux &= ~TIMER_A_MODE;
    }

    timer->regs->timer_a = timeout;

    if (timer->disable) {
        timer->regs->mux |= TIMER_A_EN;
        timer->disable = false;
    }
}

void meson_stop_timer(meson_timer_t *timer)
{
    timer->regs->mux &= ~TIMER_A_EN;
    timer->disable = true;
}
