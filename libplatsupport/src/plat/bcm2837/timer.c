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
#include <platsupport/plat/timer.h>

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

typedef struct arm_timer {
    uint32_t load;              /* Sets value for timer to count down */
    uint32_t value;             /* Holds the current timer value */
    uint32_t ctrl;              /* Control register for timer */
    uint32_t irq_clear;         /* Clears interrupt pending bit; write only */
    uint32_t raw_irq;           /* Shows status of interrupt pending bit; read only */
    uint32_t masked_irq;        /* Logical and of interrupt pending and interrupt enable */
    uint32_t reload;            /* Also timer reload value, only it doesn't force a reload */
    uint32_t pre_divider;       /* Prescaler, reset value is 0x7D  */
    uint32_t free_run_count;    /* Read only invrementing counter */
} arm_timer_t;

typedef enum {
    PERIODIC,
    ONESHOT
} sp_mode_t;

struct arm_data {
    freq_t freq;
    volatile arm_timer_t* regs;
    uint32_t prescaler;
    uint64_t counter_start;
    sp_mode_t mode;
};

static pstimer_t _timers[NTIMERS];
static struct arm_data _timer_data[NTIMERS] = {
    [SP804] = {.freq = 0, .regs = NULL },
};

#define TIMER_BASE_OFFSET 0x400

static uint32_t
sp_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    switch (n) {
        case SP804:
            return SP804_TIMER_IRQ;
        default:
            ZF_LOGE("invalid timer id %d\n", n);
        return 0;
    }
}

static int
sp_timer_start(const pstimer_t *timer)
{
    struct arm_data *data = (struct arm_data*) timer->data;

    /* Enable timer */
    data->regs->ctrl |= BIT(TIMER_ENABLE);

    return 0;
}

static int
sp_timer_stop(const pstimer_t *timer)
{
    struct arm_data *data = (struct arm_data*) timer->data;

    /* Disable timer */
    data->regs->ctrl &= ~(BIT(TIMER_ENABLE));
    return 0;
}

/* Set up the timer to fire an interrupt every ns nanoseconds.
 * The first such interrupt may arrive before ns nanoseconds
 * have passed since calling. */
static int
configure_sp(const pstimer_t *timer, uint64_t ns)
{
    struct arm_data *data = (struct arm_data*) timer->data;
    uint64_t ticks = ns / (NS_IN_US / (data->freq / MHZ));
    uint32_t prescale_bits = 0;
    data->prescaler = 1;
    data->counter_start = ns;
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
                data->prescaler = 256;
            }
        } else {
            prescale_bits = 1;
            data->prescaler = 16;
        }
    }

    /* Configure timer */
    data->regs->ctrl = 0;
    data->regs->ctrl = BIT(COUNTER_WIDTH_BIT) | (prescale_bits << PRESCALE_BIT) |
                        BIT(TIMER_INTERRUPT_ENABLE);
    data->regs->load = ticks;
    data->regs->pre_divider = 0;
    data->regs->irq_clear = 1;
    data->regs->ctrl |= BIT(TIMER_ENABLE);

    return 0;
}

static int
sp_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    volatile struct arm_data *data = (struct arm_data*) timer->data;
    data->mode = ONESHOT;
    return configure_sp(timer, ns);
}

static int
sp_periodic(const pstimer_t *timer, uint64_t ns)
{
    volatile struct arm_data *data = (struct arm_data*) timer->data;
    data->mode = PERIODIC;
    return configure_sp(timer, ns);
}

static void
sp_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    assert(irq == SP804_TIMER_IRQ);
    struct arm_data *data = (struct arm_data*) timer->data;
    if (data->regs->masked_irq) {
        data->regs->irq_clear = 1;
        if (data->mode != PERIODIC) {
            /* disable the timer if we don't want it to be periodic */
             data->regs->ctrl &= ~(BIT(TIMER_ENABLE));
        }
    } else {
        ZF_LOGD("handle irq called when no interrupt pending\n");
    }

}

static uint64_t
sp_get_time(const pstimer_t *timer)
{
    struct arm_data *data = (struct arm_data*) timer->data;
    uint64_t value;

    value = data->regs->value;
    uint64_t ns = (value / (data->freq / MHZ)) * NS_IN_US * (data->prescaler);

    return data->counter_start - ns;
}

pstimer_t *
ps_get_timer(enum timer_id id, timer_config_t *config)
{
    pstimer_t *timer;
    struct arm_data *timer_data;
    void* vaddr;
    clk_t *clk;
    /* Save the mmio address */
    vaddr = config->vaddr;

    switch (id) {
    case SP804:
        vaddr = vaddr + TIMER_BASE_OFFSET;
        break;
    default:
        return NULL;
    }

    timer = &_timers[id];
    timer_data = &_timer_data[id];
    timer->data = timer_data;
    timer_data->regs = vaddr;
    clock_sys_t clk_sys;
    clock_sys_init_default(&clk_sys);
    clk = clk_get_clock(&clk_sys, CLK_SP804);
    timer_data->freq = clk_get_freq(clk);

    timer->properties.upcounter = false;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;
    timer->properties.absolute_timeouts = false;
    timer->properties.relative_timeouts = true;
    timer->properties.periodic_timeouts = true;

    timer->start = sp_timer_start;
    timer->stop = sp_timer_stop;
    timer->get_time = sp_get_time;
    timer->oneshot_absolute = NULL;
    timer->oneshot_relative = sp_oneshot_relative;
    timer->periodic = sp_periodic;
    timer->handle_irq = sp_handle_irq;
    timer->get_nth_irq = sp_get_nth_irq;

    return timer;
}
