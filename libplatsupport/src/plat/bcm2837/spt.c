/*
 * Copyright 2017, Data61
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
#include "../../arch/arm/clock.h"
#include <platsupport/timer.h>
#include <platsupport/plat/spt.h>

/* The arm timer on the BCM2837 is based on a SP804 timer with some modifications

    - Only one timer
    - It only runs in continuous mode
    - Can be set to either stop or continue in ARM debug halt mode
    - Pre-scale options are only 1, 16 and 256
    - CLK source is 250 MHz before prescale.
    - Has an additional up counter register that will count up on each timer tick and
        roll over without generating interrupts.  It has its own prescalar.

    Functions by setting a value in the load register which the timer will move to the value
    register and when the value counts down to zero, an interrupt will occur if interrupts are
    enabled and the value from the load register will be copied back into the value register.

 */

enum {
    /*
        Width of the counter.
            0: 16-bit counters
            1: 32-bit counters
    */
        COUNTER_WIDTH_BIT = 1,
    /*
        The prescale bits select the prescale for the down counter.
        Note: There are other pre-scale bits in this register (16:23) for
                the separate free running up-counting counter.
        00 : pre-scale is clock / 1 (No pre-scale)
        01 : pre-scale is clock / 16
        10 : pre-scale is clock / 256
        11 : pre-scale is clock / 1
    */
        PRESCALE_BIT = 2,

    /*
        Interrupt enable bit.
            0: Timer interrupt disabled
            1: Timer interrupt enabled
    */
        TIMER_INTERRUPT_ENABLE = 5,

    /*
        Timer enable.
            0: Disabled
            1: Enabled
    */
        TIMER_ENABLE = 7,

    /*
        Timer behaviour when ARM is in debug halt mode:
            0: Timers keep running
            1: Timers halted.
    */
        ARM_DEBUG_HALT = 8,

    /*
        Whether the free-run counter is enabled.
            0: Enabled
            1: Disabled
     */
        FREE_RUN_ENABLE = 9,

    /*
        Free running scalar.  8 bits wide.  Reset value is 0x3E
        Freq = clk/(prescale + 1)
     */
        FREE_RUN_PRESCALE = 16
} control_reg;

#define LOAD_WIDTH 32
#define VALUE_WIDTH 32
#define CTRL_WIDTH 24
#define IRQ_CLEAR_WIDTH 32
#define RAW_IRQ_WIDTH 1
#define MASKED_IRQ_WIDTH 1
#define PRE_DIVIDER_WIDTH 10

#define TIMER_BASE_OFFSET 0x400

int spt_start(spt_t *spt)
{
    if (spt == NULL) {
        return EINVAL;
    }
    /* Enable timer */
    spt->regs->ctrl |= BIT(TIMER_ENABLE) | BIT(FREE_RUN_ENABLE);
    return 0;
}

int spt_stop(spt_t *spt)
{
    if (!spt) {
        return EINVAL;
    }
    /* Disable timer */
    spt->regs->ctrl &= ~(BIT(TIMER_ENABLE));
    return 0;
}

/* Set up the timer to fire an interrupt every ns nanoseconds.
 * The first such interrupt may arrive before ns nanoseconds
 * have passed since calling. */
int spt_set_timeout(spt_t *spt, uint64_t ns)
{
    if (!spt) {
        return EINVAL;
    }
    uint64_t ticks = ns / (NS_IN_US / (spt->freq / MHZ));
    uint32_t prescale_bits = 0;
    spt->counter_start = ns;
    if (ticks == 0) {
        ZF_LOGE("ns too low: %llu\n", ns);
        return EINVAL;
    }

    /* Prescale only has 3 values, so we need to calculate them here */
    if (ticks >= (UINT32_MAX + 1)) {
        ticks /= 16;
        if (ticks >= (1ULL << 32)) {
            ticks /= 16;
            if (ticks >= (1ULL << 32)) {
                ZF_LOGE("ns too high: %llu\n", ns);
                return EINVAL;
            } else {
                prescale_bits = 2;
            }
        } else {
            prescale_bits = 1;
        }
    }

    /* Configure timer */
    spt->regs->ctrl = 0;
    spt->regs->ctrl = BIT(COUNTER_WIDTH_BIT) | (prescale_bits << PRESCALE_BIT) |
                        BIT(TIMER_INTERRUPT_ENABLE) | BIT(FREE_RUN_ENABLE);
    spt->regs->load = ticks;
    spt->regs->pre_divider = 0;
    spt->regs->irq_clear = 1;
    spt->regs->ctrl |= BIT(TIMER_ENABLE);

    return 0;
}

int spt_handle_irq(spt_t *spt)
{
    if (!spt) {
        return EINVAL;
    }
    if (spt->regs->masked_irq) {
        spt->regs->irq_clear = 1;
        spt->regs->ctrl &= ~(BIT(TIMER_ENABLE));
    } else {
        ZF_LOGW("handle irq called when no interrupt pending\n");
        return EINVAL;
    }
    return 0;
}

uint64_t spt_get_time(spt_t *spt)
{
    uint64_t value = spt->regs->free_run_count;
    /* convert raw count to ns. As we never change the free run prescaler we do not need to
     * scale by it. We multiplly by mhz as the free run count is a 32-bit value so we will
     * not overflow a 64-bit value at that point */
    uint64_t ns = (value * MHZ / spt->freq) * NS_IN_US;
    return ns;
}

int spt_init(spt_t *spt, spt_config_t config)
{
    clk_t *clk;

    if (!spt || !config.vaddr) {
        return EINVAL;
    }

    /* Save the mmio address */
    spt->regs = (void *) ((uintptr_t) config.vaddr) + TIMER_BASE_OFFSET;
    clock_sys_t clk_sys;
    clock_sys_init_default(&clk_sys);
    clk = clk_get_clock(&clk_sys, CLK_SP804);
    spt->freq = clk_get_freq(clk);

    /* handle any pending irqs */
    spt_handle_irq(spt);
    return 0;
}
