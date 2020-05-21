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
#include <utils/time.h>

#include <platsupport/fdt.h>
#include <platsupport/io.h>
#include <platsupport/plat/dmt.h>

#include "../../ltimer.h"

/* Driver for the HiSilison hi6220 hikey Dual-timer devices.
 *
 * There are 9 timer devices, each implementing two downcounters for a total
 * of 18 downcounters. These downcounters run at 19.2MHz.
 *
 * The 9 timer devices each have their own physical frame address, but the
 * 2 downcounters for each device reside in the same 4K frame.
 *
 * We have numbered the downcounters from 0-17 as distinct logical devices.
 */

#define TCLR_ONESHOT    BIT(0)
#define TCLR_VALUE_32   BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)
#define TICKS_PER_SECOND 19200000
#define TICKS_PER_MS    (TICKS_PER_SECOND / MS_IN_S)

#define HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET (0x20)

static void dmt_timer_reset(dmt_t *dmt)
{
    assert(dmt != NULL && dmt->dmt_map != NULL);
    dmt_regs_t *dmt_regs = dmt->dmt_map;
    dmt_regs->control = 0;

    dmt->time_h = 0;
}

int dmt_stop(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    assert(dmt != NULL && dmt->dmt_map != NULL);
    dmt_regs_t *dmt_regs = dmt->dmt_map;
    dmt_regs->control = dmt_regs->control & ~TCLR_STARTTIMER;
    return 0;
}

int dmt_start(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    assert(dmt != NULL && dmt->dmt_map != NULL);
    dmt_regs_t *dmt_regs = dmt->dmt_map;
    dmt_regs->control = dmt_regs->control | TCLR_STARTTIMER;
    return 0;
}

uint64_t dmt_ticks_to_ns(uint64_t ticks)
{
    return ticks / TICKS_PER_MS * NS_IN_MS;
}

bool dmt_is_irq_pending(dmt_t *dmt)
{
    if (dmt) {
        assert(dmt != NULL && dmt->dmt_map != NULL);
        return !!dmt->dmt_map->ris;
    }
    return false;
}

int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic, bool irqs)
{
    uint64_t ticks64 = ns * TICKS_PER_MS / NS_IN_MS;
    if (ticks64 > UINT32_MAX) {
        return ETIME;
    }
    return dmt_set_timeout_ticks(dmt, ticks64, periodic, irqs);
}

int dmt_set_timeout_ticks(dmt_t *dmt, uint32_t ticks, bool periodic, bool irqs)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    assert(dmt != NULL && dmt->dmt_map != NULL);

    int flags = periodic ? TCLR_AUTORELOAD : TCLR_ONESHOT;
    flags |= irqs ? TCLR_INTENABLE : 0;

    dmt_regs_t *dmt_regs = dmt->dmt_map;
    dmt_regs->control = 0;

    /* No need to check for ticks == 0, because 0 is a valid value:
     *
     * Hikey Application Processor Function Description, section 2.3, "TIMERN_LOAD":
     *   "The minimum valid value of TIMERN_LOAD is 1. If 0 is written to TIMERN_LOAD, a
     *   timing interrupt is generated immediately."
     *
     * If the user supplies 0 as the argument, they'll just get an IRQ
     * immediately.
     */
    if (flags & TCLR_AUTORELOAD) {
        /* Hikey Application Processor Function Description, section 2.3, "TIMERN_BGLOAD":
         *   "TIMERN_BGLOAD is an initial count value register in periodic mode.
         *
         *   In periodic mode, when the value of TIMERN_BGLOAD is updated, the
         *   value of TIMERN_LOAD is changed to that of TIMERN_BGLOAD. However,
         *   the timer counter does not restart counting. After the counter
         *   decreases to 0, the value of TIMERN_LOAD (that is,
         *   the value of TIMERN_BGLOAD) is reloaded to the counter.
         *   dmt->regs->bgload = ticks;
         *
         * In other words, for periodic mode, load BGLOAD first, then write to
         * LOAD. For oneshot mode, only write to LOAD. For good measure, write 0
         * to BGLOAD.
         */
        dmt_regs->bgload = ticks;
    } else {
        dmt_regs->bgload = 0;
    }
    dmt_regs->load = ticks;

    /* The TIMERN_VALUE register is read-only. */
    dmt_regs->control = TCLR_STARTTIMER | TCLR_VALUE_32
                        | flags;

    return 0;
}

static void dmt_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    dmt_t *dmt = data;

    /* if we are being used for timestamps */
    if (dmt->user_cb_event == LTIMER_OVERFLOW_EVENT) {
        dmt->time_h++;
    }

    assert(dmt->dmt_map != NULL);
    dmt_regs_t *dmt_regs = dmt->dmt_map;
    dmt_regs->intclr = 0x1;

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (dmt->user_cb_fn) {
        dmt->user_cb_fn(dmt->user_cb_token, dmt->user_cb_event);
    }
}

uint64_t dmt_get_ticks(dmt_t *dmt)
{
    assert(dmt != NULL && dmt->dmt_map != NULL);
    dmt_regs_t *dmt_regs = dmt->dmt_map;
    return dmt_regs->value;
}

uint64_t dmt_get_time(dmt_t *dmt)
{
    uint32_t high, low;

    /* timer must be being used for timekeeping */
    assert(dmt->user_cb_event == LTIMER_OVERFLOW_EVENT);

    /* dmt is a down counter, invert the result */
    high = dmt->time_h;
    low = UINT32_MAX - dmt_get_ticks(dmt);

    /* check after fetching low to see if we've missed a high bit */
    if (dmt_is_irq_pending(dmt)) {
        high += 1;
        assert(high != 0);
    }

    uint64_t ticks = (((uint64_t) high << 32llu) | low);
    return dmt_ticks_to_ns(ticks);
}

void dmt_destroy(dmt_t *dmt)
{
    int error;
    if (dmt->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&dmt->ops.irq_ops, dmt->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (dmt->dmt_map != NULL) {
        dmt_stop(dmt);
    }
    if (dmt->dmt_map_base != NULL) {
        /* use base because dmt_map is adjusted based on whether secondary */
        ps_pmem_unmap(&dmt->ops, dmt->pmem, (void *) dmt->dmt_map_base);
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
                &dmt->dmt_map_base, &dmt->pmem, &dmt->irq_id,
                dmt_handle_irq, dmt
            );
    if (error) {
        ZF_LOGE("Simple fdt alloc helper failed");
        return error;
    }

    dmt->dmt_map = dmt->dmt_map_base;

    dmt_timer_reset(dmt);
    return 0;
}

/* initialise dmt using the base address of dmtp, so that we do not attempt to map
 * that base address again but instead re-use it. */
int dmt_init_secondary(dmt_t *dmt, dmt_t *dmtp, ps_io_ops_t ops, dmt_config_t config)
{
    int error;

    if (dmt == NULL || dmtp == NULL) {
        ZF_LOGE("dmt or dmtp cannot be null");
        return EINVAL;
    }

    dmt->ops = ops;
    dmt->user_cb_fn = config.user_cb_fn;
    dmt->user_cb_token = config.user_cb_token;
    dmt->user_cb_event = config.user_cb_event;

    /* so that destroy does not try to unmap twice */
    dmt->dmt_map_base = NULL;
    /* First sub-device is at offset 0, second sub-device is
     * at offset 0x20 within the same page. */
    dmt->dmt_map = (void *)((uintptr_t) dmtp->dmt_map_base) + HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET;
    dmt->irq_id = PS_INVALID_IRQ_ID;

    /* Gather FDT info */
    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, config.fdt_path, &cookie);
    if (error) {
        ZF_LOGE("Failed to read path (%d, %s)", error, config.fdt_path);
        return error;
    }

    /* choose irq 1 because secondary */
    irq_id_t irq_id = ps_fdt_index_register_irq(&ops, cookie, 1, dmt_handle_irq, dmt);
    if (irq_id <= PS_INVALID_IRQ_ID) {
        ZF_LOGE("Failed to register irqs (%d)", irq_id);
        return irq_id;
    }

    error = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
    if (error) {
        ZF_LOGE("Failed to clean up cookie (%d)", error);
        return error;
    }

    dmt->irq_id = irq_id;

    dmt_timer_reset(dmt);
    return 0;
}
