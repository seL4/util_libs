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

#include <platsupport/plat/dmt.h>

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

#define TCLR_ONESHOT	BIT(0)
#define TCLR_VALUE_32	BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)
#define TICKS_PER_SECOND 19200000
#define TICKS_PER_MS    (TICKS_PER_SECOND / MS_IN_S)

#define HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET (0x20)

typedef volatile struct dmt_regs {
	uint32_t load;
	uint32_t value;
	uint32_t control;
	uint32_t intclr;
	uint32_t ris;
	uint32_t mis;
	uint32_t bgload;
} dmt_regs_t;

static dmt_regs_t *get_regs(dmt_t *dmt)
{
    return dmt->regs;
}

static void dmt_timer_reset(dmt_t *dmt)
{
    dmt_regs_t *dmt_regs = get_regs(dmt);
    dmt_regs->control = 0;
}

int dmt_stop(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    dmt_regs_t *dmt_regs = get_regs(dmt);
    dmt_regs->control = dmt_regs->control & ~TCLR_STARTTIMER;
    return 0;
}

int dmt_start(dmt_t *dmt)
{
    if (dmt == NULL) {
        return EINVAL;
    }
    dmt_regs_t *dmt_regs = get_regs(dmt);
    dmt_regs->control = dmt_regs->control | TCLR_STARTTIMER;
    return 0;
}

uint64_t dmt_ticks_to_ns(uint64_t ticks) {
    return ticks / TICKS_PER_MS * NS_IN_MS;
}

bool dmt_is_irq_pending(dmt_t *dmt) {
    if (dmt) {
       return !!get_regs(dmt)->ris;
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
    int flags = periodic ? TCLR_AUTORELOAD : TCLR_ONESHOT;
    flags |= irqs ? TCLR_INTENABLE : 0;

    dmt_regs_t *dmt_regs = get_regs(dmt);
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
    dmt_regs->control = TCLR_STARTTIMER| TCLR_VALUE_32
                        | flags;

    return 0;
}

void dmt_handle_irq(dmt_t *dmt)
{
    if (dmt == NULL) {
        return;
    }
    dmt_regs_t *dmt_regs = get_regs(dmt);
    dmt_regs->intclr = 0x1;
}

uint64_t dmt_get_ticks(dmt_t *dmt)
{
    if (dmt == NULL) {
        return 0;
    }
    dmt_regs_t *dmt_regs = get_regs(dmt);
    return dmt_regs->value;
}

uint64_t dmt_get_time(dmt_t *dmt)
{
    return dmt_ticks_to_ns(dmt_get_ticks(dmt));
}

int dmt_init(dmt_t *dmt, dmt_config_t config)
{
    if (config.id > DMTIMER17) {
        ZF_LOGE("Invalid timer device ID for a hikey dual-timer.");
        return EINVAL;
    }

    if (config.vaddr == NULL || dmt == NULL) {
        ZF_LOGE("Vaddr for the mapped dual-timer device register frame is"
                " required.");
        return EINVAL;
    }

    /* Even numbered device IDs are at offset 0, odd-numbered device IDs are
     * at offset 0x20 within the same page.
     */
    if (config.id % 2 == 0) {
        dmt->regs = config.vaddr;
    } else {
        dmt->regs = (void *) ((uintptr_t ) config.vaddr) + HIKEY_DUALTIMER_SECONDARY_TIMER_OFFSET;
    }

    dmt_timer_reset(dmt);
    return 0;
}
