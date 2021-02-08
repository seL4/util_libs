/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "../../ltimer.h"

#define TIOCP_CFG_SOFTRESET BIT(0)

#define TIER_MATCHENABLE BIT(0)
#define TIER_OVERFLOWENABLE BIT(1)
#define TIER_COMPAREENABLE BIT(2)

#define TCLR_STARTTIMER BIT(0)
#define TCLR_AUTORELOAD BIT(1)
#define TCLR_PRESCALER BIT(5)
#define TCLR_COMPAREENABLE BIT(6)

#define TISR_MAT_IT_FLAG BIT(0)
#define TISR_OVF_IT_FLAG BIT(1)
#define TISR_TCAR_IT_FLAG BIT(2)

#define TISR_IRQ_CLEAR (TISR_TCAR_IT_FLAG | TISR_OVF_IT_FLAG | TISR_MAT_IT_FLAG)

static void dmt_reset(dmt_t *dmt)
{
    /* stop */
    dmt->hw->tclr = 0;
    dmt->hw->cfg = TIOCP_CFG_SOFTRESET;
    while (dmt->hw->cfg & TIOCP_CFG_SOFTRESET);
    dmt->hw->tier = TIER_OVERFLOWENABLE;

    /* reset timekeeping */
    dmt->time_h = 0;
}

int dmt_stop(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }

    dmt->hw->tclr = dmt->hw->tclr & ~TCLR_STARTTIMER;
    return 0;
}

int dmt_start(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    dmt->hw->tclr = dmt->hw->tclr | TCLR_STARTTIMER;
    return 0;
}

int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    dmt->hw->tclr = 0;      /* stop */

    /* XXX handle prescaler */
    uint32_t tclrFlags = periodic ? TCLR_AUTORELOAD : 0;

    uint64_t ticks = freq_ns_and_hz_to_cycles(ns, 24000000llu);
    if (ticks < 2) {
        return ETIME;
    }
    /* TODO: add functionality for 64 bit timeouts
     */
    if (ticks > UINT32_MAX) {
        ZF_LOGE("Timeout too far in future");
        return ETIME;
    }

    /* reload value */
    dmt->hw->tldr = 0xffffffff - (ticks);

    /* counter */
    dmt->hw->tcrr = 0xffffffff - (ticks);

    /* ack any pending irqs */
    dmt->hw->tisr = TISR_IRQ_CLEAR;
    dmt->hw->tclr = TCLR_STARTTIMER | tclrFlags;
    return 0;
}

int dmt_start_ticking_timer(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    /* stop */
    dmt->hw->tclr = 0;

    /* reset */
    dmt->hw->cfg = TIOCP_CFG_SOFTRESET;
    while (dmt->hw->cfg & TIOCP_CFG_SOFTRESET);

    /* reload value */
    dmt->hw->tldr = 0x0;

    /* use overflow mode */
    dmt->hw->tier = TIER_OVERFLOWENABLE;

    /* counter */
    dmt->hw->tcrr = 0x0;

    /* ack any pending irqs */
    dmt->hw->tisr = TISR_IRQ_CLEAR;

    /* start with auto reload */
    dmt->hw->tclr = TCLR_STARTTIMER | TCLR_AUTORELOAD;
    return 0;
}

void dmt_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    dmt_t *dmt = data;

    /* ack any pending irqs */
    dmt->hw->tisr = TISR_IRQ_CLEAR;

    /* if timer is being used for timekeeping, track overflows */
    if (dmt->user_cb_event == LTIMER_OVERFLOW_EVENT) {
        dmt->time_h++;
    }

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (dmt->user_cb_fn) {
        dmt->user_cb_fn(dmt->user_cb_token, dmt->user_cb_event);
    }
}

bool dmt_pending_overflow(dmt_t *dmt)
{
    return dmt->hw->tisr & TISR_OVF_IT_FLAG;
}

uint64_t dmt_get_time(dmt_t *dmt)
{
    uint32_t high, low;

    /* should be a timer being used for timekeeping */
    assert(dmt->user_cb_event == LTIMER_OVERFLOW_EVENT);

    high = dmt->time_h;
    low = dmt->hw->tcrr;

    /* check after fetching low to see if we've missed a high bit */
    if (dmt_pending_overflow(dmt)) {
        high += 1;
        assert(high != 0);
    }

    uint64_t ticks = (((uint64_t)high << 32llu) | low);
    return freq_cycles_and_hz_to_ns(ticks, 24000000llu);
}

void dmt_destroy(dmt_t *dmt)
{
    int error;
    if (dmt->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&dmt->ops.irq_ops, dmt->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (dmt->hw != NULL) {
        dmt_stop(dmt);
        ps_pmem_unmap(&dmt->ops, dmt->pmem, (void *) dmt->hw);
    }
}

int dmt_init(dmt_t *dmt, ps_io_ops_t ops, dmt_config_t config)
{
    int error;

    if (dmt == NULL) {
        ZF_LOGE("dmt cannot be null");
        return EINVAL;
    }

    dmt->ops = ops;
    dmt->user_cb_fn = config.user_cb_fn;
    dmt->user_cb_token = config.user_cb_token;
    dmt->user_cb_event = config.user_cb_event;

    error = helper_fdt_alloc_simple(
                &ops, config.fdt_path,
                DMT_REG_CHOICE, DMT_IRQ_CHOICE,
                (void *) &dmt->hw, &dmt->pmem, &dmt->irq_id,
                dmt_handle_irq, dmt
            );
    if (error) {
        ZF_LOGE("Failed fdt simple alloc helper");
        dmt_destroy(dmt);
        return error;
    }

    dmt_reset(dmt);
    return 0;
}
