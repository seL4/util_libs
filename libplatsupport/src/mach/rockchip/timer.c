/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The timer driver is compatible with rockchip,rk3568-timer and rk3399-timer.
 * rk3568 SoC contains 6 timers and 2 secure timers, timers 0-4 count down from a programmed value and timer 5 and 2 secure timers count up to a programmed value.
 * rk3399 SoC contains 12 timers and 12 secure timers and they all count up to a programmed value.
 * Timer0 is used as a timestamp timer and Timer1 is used as a timeout timer.
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <utils/util.h>
#include <platsupport/timer.h>
#include <platsupport/fdt.h>
#include <platsupport/mach/timer.h>
#include <utils/frequency.h>

#include "../../ltimer.h"

#define RK_REG_CHOICE 0
#define RK_IRQ_CHOICE 0
#define RK_CLOCK_FREQUENCY 24000000ull

#define TIMER_CONTROL_TIMER_ENABLE BIT(0)
#define TIMER_CONTROL_MODE_USER BIT(1)
#define TIMER_CONTROL_INTERRUPT_ENABLE BIT(2)
#define TIMER_IRQ_ACK BIT(0)

#if defined CONFIG_PLAT_RK3568
struct rk_map {
    uint32_t load_count0; /* lower bits of value to load */
    uint32_t load_count1; /* higher bits of value to load */
    uint32_t current_value0; /* lower bits of current value of the timer in ticks */
    uint32_t current_value1; /* higher bits of current value of the timer in ticks */
    uint32_t control_reg; /* control register */
    uint32_t reserved0; /* reserved */
    uint32_t int_status; /* status of the interrupt, can be cleared */
    uint32_t reserved1; /* reserved */
};
#elif defined CONFIG_PLAT_ROCKPRO64
struct rk_map {
    uint32_t load_count0; /* lower bits of value to load */
    uint32_t load_count1; /* lower bits of value to load */
    uint32_t current_value0; /* lower bits of current value of the timer in ticks */
    uint32_t current_value1; /* higher bits of current value of the timer in ticks */
    uint32_t load_count2; /* unused */
    uint32_t load_count3; /* unused */
    uint32_t int_status; /* status of the interrupt, can be cleared */
    uint32_t control_reg; /* control register */
};
#else
#error "Unsupported platform. Supported platforms are only rk3399 and rk3568!"
#endif

int rk_stop(rk_t *rk)
{
    rk->hw->control_reg = 0;
    return 0;
}

uint64_t rk_get_time(rk_t *rk)
{
    uint32_t val1 = rk->hw->current_value1;
    uint32_t val0 = rk->hw->current_value0;
    if (val1 != rk->hw->current_value1) {
        val1 = rk->hw->current_value1;
        val0 = rk->hw->current_value0;
    }

    uint64_t ticks = val0;
    ticks |= (uint64_t)val1 << 32;

#if defined CONFIG_PLAT_RK3568
    ticks = UINT64_MAX - ticks;
#endif
    return freq_cycles_and_hz_to_ns(ticks, RK_CLOCK_FREQUENCY);
}

int rk_start_timestamp_timer(rk_t *rk)
{
    rk->hw->control_reg = 0;

    /* set timer to count monotonically down for rk3568, up for rk3399 */
    rk->hw->load_count0  = 0xffffffff;
    rk->hw->load_count1  = 0xffffffff;

    rk->hw->control_reg |= TIMER_CONTROL_INTERRUPT_ENABLE | TIMER_CONTROL_TIMER_ENABLE;
    return 0;
}

int rk_set_timeout(rk_t *rk, uint64_t ns, bool periodic)
{
    uint64_t ticks = freq_ns_and_hz_to_cycles(ns, RK_CLOCK_FREQUENCY);
    uint32_t ticks_l = (uint32_t)ticks;
    uint32_t ticks_h = (uint32_t)(ticks >> 32);

    /* disable timer */
    rk->hw->control_reg = 0;

    /* reload automatically */
    uint32_t user_mode = periodic ? 0 : TIMER_CONTROL_MODE_USER;

    /* load timer count */
    rk->hw->load_count0  = ticks_l;
    rk->hw->load_count1  = ticks_h;

    /* enable timer with configs */
    rk->hw->control_reg = TIMER_CONTROL_TIMER_ENABLE | TIMER_CONTROL_INTERRUPT_ENABLE | user_mode;
    return 0;
}

static void rk_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    rk_t *rk = data;

    /* ack any pending irqs */
    rk->hw->int_status = TIMER_IRQ_ACK;

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (rk->user_cb_fn) {
        rk->user_cb_fn(rk->user_cb_token, rk->user_cb_event);
    }
}

void rk_destroy(rk_t *rk)
{
    int error;
    if (rk->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&rk->ops.irq_ops, rk->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (rk->hw != NULL) {
        rk_stop(rk);
    }
    if (rk->rk_map_base != NULL) {
        /* use base because rk_map is adjusted based on whether secondary */
        ps_pmem_unmap(&rk->ops, rk->pmem, (void *) rk->rk_map_base);
    }
}

static int irq_index_walker(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    rk_t *rk = token;

    if (RK_IRQ_CHOICE == curr_num) {
        irq_id_t registered_id = ps_irq_register(&rk->ops.irq_ops, irq, rk_handle_irq, rk);
        if (registered_id >= 0) {
            rk->irq_id = registered_id;
            rk->irq = irq;
        } else {
            /* Bail on error */
            return registered_id;
        }
    }

    return 0;
}

int rk_init(rk_t *rk, ps_io_ops_t ops, rk_config_t config)
{
    int error;

    if (rk == NULL) {
        ZF_LOGE("rk cannot be null");
        return EINVAL;
    }

    rk->ops = ops;
    rk->user_cb_fn = config.user_cb_fn;
    rk->user_cb_token = config.user_cb_token;
    rk->user_cb_event = config.user_cb_event;

    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, config.fdt_path, &cookie);
    if (error) {
        ZF_LOGE("rk3xxx timer failed to read path (%d, %s)", error, config.fdt_path);
        return error;
    }

    rk->rk_map_base = ps_fdt_index_map_register(&ops, cookie, RK_REG_CHOICE, &rk->pmem);
    if (rk->rk_map_base == NULL) {
        ZF_LOGE("rk3xxx timer failed to map registers");
        return ENODEV;
    }

    error = ps_fdt_walk_irqs(&ops.io_fdt, cookie, irq_index_walker, rk);
    if (error) {
        ZF_LOGE("rk3xxx timer failed to register irqs (%d)", error);
        return error;
    }

    rk->irq_id = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
    if (rk->irq_id) {
        ZF_LOGE("rk3xxx timer to clean up cookie (%d)", error);
        return rk->irq_id;
    }

    rk->hw = rk->rk_map_base;

    return 0;
}

/* initialise rk using the base address of rkp, so that we do not attempt to map
 * that base address again but instead re-use it. */
int rk_init_secondary(rk_t *rk, rk_t *rkp, ps_io_ops_t ops, rk_config_t config)
{
    int error;

    if (rk == NULL || rkp == NULL) {
        ZF_LOGE("rk or rkp cannot be null");
        return EINVAL;
    }

    rk->ops = ops;
    rk->user_cb_fn = config.user_cb_fn;
    rk->user_cb_token = config.user_cb_token;
    rk->user_cb_event = config.user_cb_event;

    /* so that destroy does not try to unmap twice */
    rk->rk_map_base = NULL;
    /* just like dmt, rk3xxx has another timer in the same page at 0x20 offset */
    rk->hw = (void *)((uintptr_t) rkp->rk_map_base) + 0x20;
    /* similarly, the IRQ for this secondary timer is offset by 1 */
    rk->irq_id = PS_INVALID_IRQ_ID;
    ps_irq_t irq2 = { .type = PS_INTERRUPT, .irq.number = rkp->irq.irq.number + 1 };
    irq_id_t irq2_id = ps_irq_register(&ops.irq_ops, irq2, rk_handle_irq, rk);
    if (irq2_id < 0) {
        ZF_LOGE("Failed to register secondary irq for rk timer");
        return irq2_id;
    }
    rk->irq_id = irq2_id;

    return 0;
}
