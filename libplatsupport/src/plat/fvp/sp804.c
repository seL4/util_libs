/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/plat/sp804.h>

#include "../../ltimer.h"

/* This file is mostly the same as the dmt.c file for the hikey.
 * Consider to merge the two files as a single driver file for
 * SP804.
 */

#define TCLR_ONESHOT    BIT(0)
#define TCLR_VALUE_32   BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)
/* It looks like the FVP does not emulate time accruately. Thus, pick
 *  a small Hz that triggers interrupts in a reasonable time */
#define TICKS_PER_SECOND 35000
#define TICKS_PER_MS    (TICKS_PER_SECOND / MS_IN_S)

static void sp804_timer_reset(sp804_t *sp804)
{
    assert(sp804 != NULL && sp804->sp804_map != NULL);
    sp804_regs_t *sp804_regs = sp804->sp804_map;
    sp804_regs->control = 0;

    sp804->time_h = 0;
}

int sp804_stop(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return EINVAL;
    }
    assert(sp804->sp804_map != NULL);
    sp804_regs_t *sp804_regs = sp804->sp804_map;
    sp804_regs->control = sp804_regs->control & ~TCLR_STARTTIMER;
    return 0;
}

int sp804_start(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return EINVAL;
    }
    assert(sp804->sp804_map != NULL);
    sp804_regs_t *sp804_regs = sp804->sp804_map;
    sp804_regs->control = sp804_regs->control | TCLR_STARTTIMER;
    return 0;
}

uint64_t sp804_ticks_to_ns(uint64_t ticks)
{
    return ticks / TICKS_PER_MS * NS_IN_MS;
}

bool sp804_is_irq_pending(sp804_t *sp804)
{
    if (sp804) {
        assert(sp804->sp804_map != NULL);
        return !!sp804->sp804_map->ris;
    }
    return false;
}

int sp804_set_timeout(sp804_t *sp804, uint64_t ns, bool periodic, bool irqs)
{
    uint64_t ticks64 = ns * TICKS_PER_MS / NS_IN_MS;
    if (ticks64 > UINT32_MAX) {
        return ETIME;
    }
    return sp804_set_timeout_ticks(sp804, ticks64, periodic, irqs);
}

int sp804_set_timeout_ticks(sp804_t *sp804, uint32_t ticks, bool periodic, bool irqs)
{
    if (sp804 == NULL) {
        return EINVAL;
    }
    int flags = periodic ? TCLR_AUTORELOAD : TCLR_ONESHOT;
    flags |= irqs ? TCLR_INTENABLE : 0;

    assert(sp804->sp804_map != NULL);
    sp804_regs_t *sp804_regs = sp804->sp804_map;
    sp804_regs->control = 0;

    if (flags & TCLR_AUTORELOAD) {
        sp804_regs->bgload = ticks;
    } else {
        sp804_regs->bgload = 0;
    }
    sp804_regs->load = ticks;

    /* The TIMERN_VALUE register is read-only. */
    sp804_regs->control = TCLR_STARTTIMER | TCLR_VALUE_32
                          | flags;

    return 0;
}

static void sp804_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    sp804_t *sp804 = data;

    if (sp804->user_cb_event == LTIMER_OVERFLOW_EVENT) {
        sp804->time_h++;
    }

    sp804_regs_t *sp804_regs = sp804->sp804_map;
    sp804_regs->intclr = 0x1;

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (sp804->user_cb_fn) {
        sp804->user_cb_fn(sp804->user_cb_token, sp804->user_cb_event);
    }
}

uint64_t sp804_get_ticks(sp804_t *sp804)
{
    assert(sp804 != NULL && sp804->sp804_map != NULL);
    sp804_regs_t *sp804_regs = sp804->sp804_map;
    return sp804_regs->value;
}

uint64_t sp804_get_time(sp804_t *sp804)
{
    uint32_t high, low;

    /* timer must be being used for timekeeping */
    assert(sp804->user_cb_event == LTIMER_OVERFLOW_EVENT);

    /* sp804 is a down counter, invert the result */
    high = sp804->time_h;
    low = UINT32_MAX - sp804_get_ticks(sp804);

    /* check after fetching low to see if we've missed a high bit */
    if (sp804_is_irq_pending(sp804)) {
        high += 1;
        assert(high != 0);
    }

    uint64_t ticks = (((uint64_t) high << 32llu) | low);
    return sp804_ticks_to_ns(ticks);
}

void sp804_destroy(sp804_t *sp804)
{
    int error;
    if (sp804->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&sp804->ops.irq_ops, sp804->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (sp804->sp804_map != NULL) {
        sp804_stop(sp804);
        ps_pmem_unmap(&sp804->ops, sp804->pmem, (void *) sp804->sp804_map);
    }
}

int sp804_init(sp804_t *sp804, ps_io_ops_t ops, sp804_config_t config)
{
    int error;

    if (sp804 == NULL) {
        ZF_LOGE("sp804 cannot be null");
        return EINVAL;
    }

    sp804->ops = ops;
    sp804->user_cb_fn = config.user_cb_fn;
    sp804->user_cb_token = config.user_cb_token;
    sp804->user_cb_event = config.user_cb_event;

    error = helper_fdt_alloc_simple(
                &ops, config.fdt_path,
                SP804_REG_CHOICE, SP804_IRQ_CHOICE,
                (void *) &sp804->sp804_map, &sp804->pmem, &sp804->irq_id,
                sp804_handle_irq, sp804
            );
    if (error) {
        ZF_LOGE("Simple fdt alloc helper failed");
        return error;
    }

    sp804_timer_reset(sp804);
    return 0;
}
