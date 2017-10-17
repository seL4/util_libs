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

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define TIOCP_CFG_SOFTRESET BIT(0)

#define TIER_MATCHENABLE BIT(0)
#define TIER_OVERFLOWENABLE BIT(1)
#define TIER_COMPAREENABLE BIT(2)

#define TCLR_AUTORELOAD BIT(1)
#define TCLR_COMPAREENABLE BIT(6)
#define TCLR_STARTTIMER BIT(0)

#define TISR_TCAR_IT_FLAG BIT(2)
#define TISR_OVF_IT_FLAG BIT(1)
#define TISR_MAT_IT_FLAG BIT(0)

#define TISR_IRQ_CLEAR (TISR_TCAR_IT_FLAG | TISR_OVF_IT_FLAG | TISR_MAT_IT_FLAG)

#define TICKS_PER_SECOND 24000000  /* TODO: Pin this frequency down without relying on u-boot. */
#define TIMER_INTERVAL_TICKS(ns) ((uint32_t)(1ULL * (ns) * TICKS_PER_SECOND / 1000 / 1000 / 1000))
#define TICKS_TIMER_INTERVAL(x)  (((uint64_t)x * 1000 * 1000 * 1000) / TICKS_PER_SECOND)

static void dmt_reset(dmt_t *dmt)
{
    dmt->hw->tclr = 0;          /* stop */
    dmt->hw->cfg = TIOCP_CFG_SOFTRESET;
    while (dmt->hw->cfg & TIOCP_CFG_SOFTRESET);
    dmt->hw->tier = TIER_OVERFLOWENABLE;
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

    uint32_t ticks = TIMER_INTERVAL_TICKS(ns);
    if (ticks < 2) {
        return EINVAL;
    }
    //printf("timer %lld ns = %x ticks (cntr %x)\n", ns, ticks, (uint32_t)(~0UL - ticks));
    dmt->hw->tldr = ~0UL - ticks;   /* reload value */
    dmt->hw->tcrr = ~0UL - ticks;   /* counter */
    dmt->hw->tisr = TISR_IRQ_CLEAR;  /* ack any pending irqs */
    dmt->hw->tclr = TCLR_STARTTIMER | tclrFlags;
    return 0;
}

void dmt_handle_irq(dmt_t *dmt)
{
    if (dmt == NULL) {
        ZF_LOGE("DMT is NULL");
        return;
    }
    dmt->hw->tisr = TISR_IRQ_CLEAR;  /* ack any pending irqs */
}

bool dmt_pending_match(dmt_t *dmt)
{
    return dmt->hw->tisr & TISR_MAT_IT_FLAG;
}

int dmt_init(dmt_t *dmt, dmt_config_t config)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    if (config.id < DMTIMER2 || config.id >= NTIMERS) {
        ZF_LOGE("Invalid timer id");
        return EINVAL;
    }

    dmt->hw = (struct dmt_map *)config.vaddr;
    // XXX support config->prescaler.

    dmt_reset(dmt);
    return 0;
}
