/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <platsupport/mach/timer.h>
#include <stdint.h>
#include <utils/util.h>

#define MAX_TIMEOUT_NS (SPACEMIT_TIMER_MAX_TICKS * (NS_IN_S / SPACEMIT_TIMER_TICKS_PER_SECOND))

static void spacemit_timer_enable_clocks(spacemit_timer_t *timer)
{
    volatile uint32_t *base   = (volatile uint32_t *)((uint8_t *)timer->vaddr + 0x1000); // APB_CLK_BASE
    uint32_t offset           = APBC_TIMERx_CLK_RST_OFFSET / sizeof(uint32_t);
    uint32_t clk_select_shift = 0x4;
    uint32_t init_value       = 0x0;

    init_value |= (1 << 0);
    init_value |= (1 << 1);
    init_value |= K1_TCCR_CS0_VALUE << clk_select_shift;

    base[offset] = init_value;
}

void spacemit_timer_enable(spacemit_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);
    assert(timer->timer_n < SPACEMIT_NUM_TIMERS);

    timer->regs->tcer  |= (1u << timer->timer_n);
}


void spacemit_timer_disable(spacemit_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->tcer  &= ~(1u << timer->timer_n);
}

uint64_t spacemit_timer_get_time(spacemit_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    uint8_t n = timer->timer_n;
    /* the timer counter counts up */
    uint64_t value_l = (uint64_t)timer->regs->tcnt[n];
    uint64_t value_h = (uint64_t)timer->value_h;

    /* Include unhandled interrupt in value_h */
    uint32_t status = timer->regs->tsr[n];
    if (status & 0x1) {
        value_h += 1;
    }

    uint64_t value_ticks = (value_h << 32) | value_l;

    /* convert from ticks to nanoseconds */
    uint64_t whole_seconds = value_ticks / SPACEMIT_TIMER_TICKS_PER_SECOND;
    uint64_t subsecond_ticks = value_ticks % SPACEMIT_TIMER_TICKS_PER_SECOND;
    uint64_t subsecond_ns =
        (subsecond_ticks * NS_IN_S) / SPACEMIT_TIMER_TICKS_PER_SECOND;
    uint64_t value_ns = whole_seconds * NS_IN_S + subsecond_ns;

    return value_ns;
}

void spacemit_timer_reset(spacemit_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    spacemit_timer_disable(timer);
    timer->value_h = 0;
    uint8_t n = timer->timer_n;

    /* Reset match register */
    timer->regs->tmr[n][0] = (uint32_t)(SPACEMIT_TIMER_MAX_TICKS);
}

int spacemit_timer_set_timeout(spacemit_timer_t *timer, uint64_t ns, bool is_periodic)
{
    spacemit_timer_disable(timer);
    timer->value_h = 0;
    uint8_t n = timer->timer_n;

    if (is_periodic) {
        timer->regs->tcmr &= ~(1u << n);      /* Periodic mode */
    } else {
        timer->regs->tcmr |= (1u << n);       /* Free-run mode */
    }

    /* convert from nanoseconds to ticks */
    uint64_t ticks_whole_seconds = (ns / NS_IN_S) * SPACEMIT_TIMER_TICKS_PER_SECOND;
    uint64_t ticks_remainder = (ns % NS_IN_S) * SPACEMIT_TIMER_TICKS_PER_SECOND / NS_IN_S;
    uint64_t num_ticks = ticks_whole_seconds + ticks_remainder;

    if (num_ticks > SPACEMIT_TIMER_MAX_TICKS) {
        ZF_LOGE("Requested timeout of %"PRIu64" ns exceeds hardware limit of %"PRIu64" ns",
                ns,
                MAX_TIMEOUT_NS);
        return -EINVAL;
    }

    /* Set the match register and enable interrupt */
    timer->regs->tmr[n][0] = (uint32_t)(num_ticks);
    timer->regs->tier[n]   |= (1u << 0);

    spacemit_timer_enable(timer);

    return 0;
}


void spacemit_timer_disable_all(void *vaddr)
{
    assert(vaddr);
    volatile spacemit_timer_regs_t *regs = vaddr;
    regs->tcer     &= ~(0x7u);

}

void spacemit_timer_init(spacemit_timer_t *timer, void *vaddr, size_t n)
{
    assert(timer);
    assert(vaddr);
    assert(n < SPACEMIT_NUM_TIMERS);

    timer->vaddr = (uint32_t *)vaddr;

    spacemit_timer_enable_clocks(timer);
    timer->regs           = vaddr;  /* all timers share same base addr */
    timer->timer_n        = n;
    timer->value_h        = 0;
    timer->regs->tcer     &= ~(1u << n);
    timer->regs->tcmr     |= (1u << n);       /* Free-run mode */

    uint32_t clock_ctrl   = timer->regs->tccr;
    clock_ctrl            &= ~(3U << (n * 2));      /* Clear existing bits */
    clock_ctrl            |= (0x0 << (n * 2));      /* Set to fast clock (12.8 MHz) */
    timer->regs->tccr     = clock_ctrl;

    /* Set the match register and enable interrupt */
    timer->regs->tmr[n][0] = (uint32_t)(SPACEMIT_TIMER_MAX_TICKS);
    timer->regs->tier[n]  |= (1u << 0);
}
