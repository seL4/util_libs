/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>

#include <platsupport/fdt.h>
#include <platsupport/plat/system_timer.h>
#include <platsupport/mach/system_timer.h>
#include <platsupport/io.h>
#include <platsupport/pmem.h>

#include "../../ltimer.h"

static inline uint64_t bcm_system_timer_read_ticks(volatile bcm_system_timer_registers_t *registers)
{
    // no atomic read, so first read high, then low, then high again. If high
    // changed, then low overflowed and low should be re-read.
    uint64_t high_initial = registers->counter_high;
    uint64_t low = registers->counter_low;
    uint64_t high = registers->counter_high;
    if (high != high_initial) {
        low = registers->counter_low;
    }

    return (high << 32) | low;
}

static inline bool bcm_system_timer_triggered(volatile bcm_system_timer_registers_t *registers,
                                              uint32_t channel)
{
    return !!(registers->control & BIT(channel));
}

static inline void bcm_system_timer_clear_irq(volatile bcm_system_timer_registers_t *registers,
                                              uint32_t channel)
{
    registers->control = BIT(channel);
}

static inline int bcm_system_timer_timeout_valid(uint64_t freq, uint64_t ns)
{
    /* only 32bit timeouts supported */
    return (freq_ns_and_hz_to_cycles(ns, freq) <= UINT32_MAX);
}

static int bcm_system_timer_set_compare(bcm_system_timer_t *timer,
                                        uint64_t ticks)
{
    if (!timer) {
        return EINVAL;
    }

    /* Clear any existing interrupt. */
    bcm_system_timer_clear_irq(timer->registers, timer->channel);
    timer->registers->compare[timer->channel] = (uint32_t)(MASK(32) & ticks);

    /* Make sure timeout was not missed */
    if (bcm_system_timer_read_ticks(timer->registers) >= ticks &&
        !bcm_system_timer_triggered(timer->registers, timer->channel)) {
        /* disable timer */
        bcm_system_timer_clear_irq(timer->registers, timer->channel);
        ZF_LOGE("timeout missed");
        return ETIME;
    }

    return 0;
}

static int bcm_system_timer_set_rel_timeout(bcm_system_timer_t *timer,
                                            uint64_t ns)
{
    if (!timer) {
        return EINVAL;
    }

    if (!bcm_system_timer_timeout_valid(timer->frequency, ns)) {
        ZF_LOGE("unsupported timeout length");
        return EINVAL;
    }

    uint64_t now = bcm_system_timer_read_ticks(timer->registers);
    uint64_t until = now + freq_ns_and_hz_to_cycles(ns, timer->frequency);

    if (until <= now) {
        ZF_LOGE("timeout already passed");
        return ETIME;
    }

    return bcm_system_timer_set_compare(timer, until);
}

static void bcm_system_timer_handle_irq(void *data,
                                        ps_irq_acknowledge_fn_t acknowledge_fn,
                                        void *ack_data)
{
    if (!data) {
        ZF_LOGF("No IRQ data");
        return;
    }

    bcm_system_timer_t *timer = data;

    if (bcm_system_timer_triggered(timer->registers, timer->channel)) {
        bcm_system_timer_clear_irq(timer->registers, timer->channel);

        if (timer->period) {
            bcm_system_timer_set_timeout(timer, timer->period, TIMEOUT_PERIODIC);
        }
    }

    if (timer->callback) {
        timer->callback(timer->callback_token, LTIMER_TIMEOUT_EVENT);
    }

    ZF_LOGF_IF(acknowledge_fn(ack_data), "failed to acknowledge IRQ");
}

int bcm_system_timer_get_time(bcm_system_timer_t *timer, uint64_t *time)
{
    if (!timer || !time) {
        return EINVAL;
    }

    *time = freq_cycles_and_hz_to_ns(bcm_system_timer_read_ticks(timer->registers),
                                     timer->frequency);
    return 0;
}

int bcm_system_timer_set_timeout(bcm_system_timer_t *timer,
                                 uint64_t ns,
                                 timeout_type_t type)
{
    int rc = 0;
    uint64_t now = 0;

    if (!timer) {
        return EINVAL;
    }

    timer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        /* to relative timeout */
        rc = bcm_system_timer_get_time(timer, &now);
        if (rc) {
            return rc;
        }

        if (ns <= now) {
            ZF_LOGE("timeout passed");
            return ETIME;
        }
        ns -= now;
        break;
    case TIMEOUT_PERIODIC:
        timer->period = ns;
    // fall through
    case TIMEOUT_RELATIVE:
        break;
    default:
        ZF_LOGE("unsupported timeout type");
        return EINVAL;
    }

    return bcm_system_timer_set_rel_timeout(timer, ns);
}

int bcm_system_timer_reset(bcm_system_timer_t *timer)
{
    if (!timer) {
        return EINVAL;
    }

    timer->period = 0;
    bcm_system_timer_clear_irq(timer->registers, timer->channel);

    return 0;
}

int bcm_system_timer_init(bcm_system_timer_t *timer,
                          ps_io_ops_t ops,
                          ltimer_callback_fn_t callback,
                          void *callback_token,
                          bcm_system_timer_config_t config)
{
    if (!timer || !config.fdt_path) {
        return EINVAL;
    }

    timer->frequency = config.frequency;
    timer->channel = config.channel;
    timer->ops = ops;
    timer->callback = callback;
    timer->callback_token = callback_token;

    int rc = helper_fdt_alloc_simple(&timer->ops,
                                     config.fdt_path,
                                     config.fdt_reg_choice,
                                     config.fdt_irq_choice,
                                     (void *) &timer->registers,
                                     &timer->pmem,
                                     &timer->irq,
                                     bcm_system_timer_handle_irq,
                                     timer);

    if (rc) {
        ZF_LOGE("Failed to allocate with fdt");
        bcm_system_timer_destroy(timer);
        return rc;
    }

    rc = bcm_system_timer_reset(timer);
    if (rc) {
        ZF_LOGE("Timer reset failed");
        bcm_system_timer_destroy(timer);
    }

    return rc;
}

void bcm_system_timer_destroy(bcm_system_timer_t *timer)
{
    if (!timer) {
        return;
    }

    if (timer->registers) {
        bcm_system_timer_reset(timer);
    }

    if (timer->irq > PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&timer->ops.irq_ops, timer->irq),
                   "Failed to unregister IRQ ID");
        timer->irq = PS_INVALID_IRQ_ID;
    }

    if (timer->registers) {
        ps_pmem_unmap(&timer->ops, timer->pmem, (void *)timer->registers);
        timer->registers = NULL;
    }
}
