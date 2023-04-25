/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <platsupport/plat/timer.h>
#include <stdint.h>
#include <utils/util.h>

#define MAX_TIMEOUT_NS (STARFIVE_TIMER_MAX_TICKS * (NS_IN_S / STARFIVE_TIMER_TICKS_PER_SECOND))

static void print_regs(starfive_timer_t *timer)
{
    printf("   Timer Channel Register State\n");
    printf("     Interrupt status: 0x%08x @ [%p]\n", timer->regs->status, &timer->regs->status);
    printf("     Control:          0x%08x @ [%p]\n", timer->regs->ctrl, &timer->regs->ctrl);
    printf("     Load:             0x%08x @ [%p]\n", timer->regs->load, &timer->regs->load);
    printf("     Enable:           0x%08x @ [%p]\n", timer->regs->enable, &timer->regs->enable);
    printf("     Reload:           0x%08x @ [%p]\n", timer->regs->reload, &timer->regs->reload);
    printf("     Value:            0x%08x @ [%p]\n", timer->regs->value, &timer->regs->value);
    printf("     Clear Interrupt:  0x%08x @ [%p]\n", timer->regs->intclr, &timer->regs->intclr);
    printf("     Mask Interrupt:   0x%08x @ [%p]\n", timer->regs->intmask, &timer->regs->intmask);
}

void starfive_timer_start(starfive_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->enable = STARFIVE_TIMER_ENABLED;
}

void starfive_timer_stop(starfive_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->enable = STARFIVE_TIMER_DISABLED;
}

void starfive_timer_handle_irq(starfive_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->value_h += 1;

    while (timer->regs->intclr & STARFIVE_TIMER_INTCLR_BUSY) {
        /*
         * Hardware will not currently accept writes to this register.
         * Wait for this bit to be unset by hardware.
         */
    }

    timer->regs->intclr = 1;
}

uint64_t starfive_timer_get_time(starfive_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    /* the timer value counts down from the load value */
    uint64_t value_l = (uint64_t)(STARFIVE_TIMER_MAX_TICKS - timer->regs->value);
    uint64_t value_h = (uint64_t)timer->value_h;

    /* Include unhandled interrupt in value_h */
    if (timer->regs->intclr == 1) {
        value_h += 1;
    }

    uint64_t value_ticks = (value_h << 32) | value_l;

    /* convert from ticks to nanoseconds */
    uint64_t value_whole_seconds = value_ticks / STARFIVE_TIMER_TICKS_PER_SECOND;
    uint64_t value_subsecond_ticks = value_ticks % STARFIVE_TIMER_TICKS_PER_SECOND;
    uint64_t value_subsecond_ns =
        (value_subsecond_ticks * NS_IN_S) / STARFIVE_TIMER_TICKS_PER_SECOND;
    uint64_t value_ns = value_whole_seconds * NS_IN_S + value_subsecond_ns;

    return value_ns;
}

void starfive_timer_reset(starfive_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);
    assert(timer->regs->enable == STARFIVE_TIMER_DISABLED);

    timer->regs->ctrl = STARFIVE_TIMER_MODE_CONTINUOUS;
    timer->regs->load = STARFIVE_TIMER_MAX_TICKS;
    timer->value_h = 0;
}

int starfive_timer_set_timeout(starfive_timer_t *timer, uint64_t ns, bool is_periodic)
{
    starfive_timer_stop(timer);
    timer->value_h = 0;

    if (is_periodic) {
        timer->regs->ctrl = STARFIVE_TIMER_MODE_CONTINUOUS;
    } else {
        timer->regs->ctrl = STARFIVE_TIMER_MODE_SINGLE;
    }

    uint64_t ticks_whole_seconds = (ns / NS_IN_S) * STARFIVE_TIMER_TICKS_PER_SECOND;
    uint64_t ticks_remainder = (ns % NS_IN_S) * STARFIVE_TIMER_TICKS_PER_SECOND / NS_IN_S;
    uint64_t num_ticks = ticks_whole_seconds + ticks_remainder;

    if (num_ticks > STARFIVE_TIMER_MAX_TICKS) {
        ZF_LOGE("Requested timeout of %"PRIu64" ns exceeds hardware limit of %"PRIu64" ns",
                ns,
                MAX_TIMEOUT_NS);
        return -EINVAL;
    }

    timer->regs->load = num_ticks;
    starfive_timer_start(timer);

    return 0;
}

void starfive_timer_disable_all_channels(void *vaddr)
{
    assert(vaddr);

    for (int x = 0; x < STARFIVE_TIMER_NUM_CHANNELS; x++) {
        uint32_t *enable = vaddr + 0x10 + x * STARFIVE_TIMER_CHANNEL_REGISTERS_LEN_IN_BYTES;
        *enable = STARFIVE_TIMER_DISABLED;
    }
}

void starfive_timer_init(starfive_timer_t *timer, void *vaddr, uint64_t channel)
{
    assert(timer);
    assert(vaddr);
    assert(channel >= 0 && channel < STARFIVE_TIMER_NUM_CHANNELS);

    timer->regs = vaddr + STARFIVE_TIMER_CHANNEL_REGISTERS_LEN_IN_BYTES * channel;
    timer->regs->enable = STARFIVE_TIMER_DISABLED;
    timer->regs->ctrl = STARFIVE_TIMER_MODE_CONTINUOUS;
    timer->regs->load = STARFIVE_TIMER_MAX_TICKS;
    timer->regs->intmask = STARFIVE_TIMER_INTERRUPT_UNMASKED;
    timer->value_h = 0;
}
