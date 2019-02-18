/*
 * Copyright 2019, Data61
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

#include <platsupport/plat/sp804.h>

/* This file is mostly the same as the dmt.c file for the hikey.
 * Consider to merge the two files as a single driver file for
 * SP804.
 */

#define TCLR_ONESHOT	BIT(0)
#define TCLR_VALUE_32	BIT(1)
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)
/* It looks like the FVP does not emulate time accruately. Thus, pick
 *  a small Hz that triggers interrupts in a reasonable time */
#define TICKS_PER_SECOND 35000 
#define TICKS_PER_MS    (TICKS_PER_SECOND / MS_IN_S)

typedef volatile struct sp804_regs {
	uint32_t load;
	uint32_t value;
	uint32_t control;
	uint32_t intclr;
	uint32_t ris;
	uint32_t mis;
	uint32_t bgload;
} sp804_regs_t;

static sp804_regs_t *get_regs(sp804_t *sp804)
{
    return sp804->regs;
}

static void sp804_timer_reset(sp804_t *sp804)
{
    sp804_regs_t *sp804_regs = get_regs(sp804);
    sp804_regs->control = 0;
}

int sp804_stop(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return EINVAL;
    }
    sp804_regs_t *sp804_regs = get_regs(sp804);
    sp804_regs->control = sp804_regs->control & ~TCLR_STARTTIMER;
    return 0;
}

int sp804_start(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return EINVAL;
    }
    sp804_regs_t *sp804_regs = get_regs(sp804);
    sp804_regs->control = sp804_regs->control | TCLR_STARTTIMER;
    return 0;
}

uint64_t sp804_ticks_to_ns(uint64_t ticks) {
    return ticks / TICKS_PER_MS * NS_IN_MS;
}

bool sp804_is_irq_pending(sp804_t *sp804) {
    if (sp804) {
       return !!get_regs(sp804)->ris;
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

    sp804_regs_t *sp804_regs = get_regs(sp804);
    sp804_regs->control = 0;

    if (flags & TCLR_AUTORELOAD) {
        sp804_regs->bgload = ticks;
    } else {
        sp804_regs->bgload = 0;
    }
    sp804_regs->load = ticks;

    /* The TIMERN_VALUE register is read-only. */
    sp804_regs->control = TCLR_STARTTIMER| TCLR_VALUE_32
                        | flags;

    return 0;
}

void sp804_handle_irq(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return;
    }
    sp804_regs_t *sp804_regs = get_regs(sp804);
    sp804_regs->intclr = 0x1;
}

uint64_t sp804_get_ticks(sp804_t *sp804)
{
    if (sp804 == NULL) {
        return 0;
    }
    sp804_regs_t *sp804_regs = get_regs(sp804);
    return sp804_regs->value;
}

uint64_t sp804_get_time(sp804_t *sp804)
{
    return sp804_ticks_to_ns(sp804_get_ticks(sp804));
}

int sp804_init(sp804_t *sp804, sp804_config_t config)
{
    if (config.id > SP804_TIMER1) {
        ZF_LOGE("Invalid timer device ID for a hikey dual-timer.");
        return EINVAL;
    }

    if (config.vaddr == NULL || sp804 == NULL) {
        ZF_LOGE("Vaddr for the mapped dual-timer device register frame is"
                " required.");
        return EINVAL;
    }

    sp804->regs = config.vaddr;

    sp804_timer_reset(sp804);
    return 0;
}
