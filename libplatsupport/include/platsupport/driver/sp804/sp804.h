/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <utils/arith.h>


#define TCLR_ONESHOT    BIT(0)
#define TCLR_VALUE_32   BIT(1)
/* Bit 2 and 3: Prescaler
 *              00 = clock is divided by 1 (default)
 *              01 = clock is divided by 16
 *              10 = clock is divided by 256
 *              11 = reserved, do not use.
 * Bit 4: reserved
 */
#define TCLR_INTENABLE  BIT(5)
#define TCLR_AUTORELOAD BIT(6)
#define TCLR_STARTTIMER BIT(7)

/*
 * The sp840 contains two identical timers:
 *
 *    0x000 - 0x01f   sp804_regs_t timer1;
 *    0x020 - 0x03f   sp804_regs_t timer2;
 *    0x040 - 0xefc   Reserved
 *    0xf00           TimerITCR
 *    0xf04           TimerITOP
 *    0xf08 - 0xfdf   Reserved
 *    0xfe0 - 0xfef   TimerPeriphID[4]
 *    0xff0 - 0xfff   TimerPCellID[4]
 */

#define SP804_TIMER2_OFFSET 0x20

typedef volatile struct {
    uint32_t load;      /* 0x00 */
    uint32_t value;     /* 0x04 */
    uint32_t control;   /* 0x08 TCLR */
    uint32_t intclr;    /* 0x0c Interrupt Clear */
    uint32_t ris;       /* 0x10 Raw interrupt Status */
    uint32_t mis;       /* 0x14 Masked Interrupt Status */
    uint32_t bgload;    /* 0x18 Background Load */
    uint32_t _rfu;      /* 0x1c (unused) */
} sp804_regs_t;

compile_time_assert(sp804_timer_size , sizeof(sp804_regs_t) <= SP804_TIMER2_OFFSET);

#define SP804_TIMER1_REGS(base)  ((sp804_regs_t *)(base))
#define SP804_TIMER2_REGS(base)  ((sp804_regs_t *)((uintptr_t)(base) + SP804_TIMER2_OFFSET))

static void sp804_reset(sp804_regs_t *regs)
{
    regs->control = 0;
}

static void sp804_stop(sp804_regs_t *regs)
{
    regs->control &= ~TCLR_STARTTIMER;
}

static void sp804_start(sp804_regs_t *regs)
{
    regs->control |= TCLR_STARTTIMER;
}

static bool sp804_is_irq_pending(sp804_regs_t *regs)
{
    /* return the raw interrupt status and not the masted interrupt status */
    return !!regs->ris;
}

static bool sp804_clear_intr(sp804_regs_t *regs)
{
    regs->intclr = 0x1;
}

static uint32_t sp804_get_ticks(sp804_regs_t *regs)
{
    return regs->value;
}

static void sp804_set_timeout(sp804_regs_t *regs, uint32_t ticks,
                              bool is_periodic, bool enable_intr)
{
    regs->control = 0; /* stop timer */

    /* If 'ticks' is 0, then writing to 'load' will generate an interrupt
     * immediately.
     *
     * The "Hikey Application Processor Function Description" says in
     * section 2.3:
     *   The minimum valid value of TIMERN_LOAD is 1. If 0 is written to
     *   TIMERN_LOAD, a timing interrupt is generated immediately.
     *   TIMERN_BGLOAD is an initial count value register in periodic mode. In
     *   periodic mode, when the value of TIMERN_BGLOAD is updated, the
     *   value of TIMERN_LOAD is changed to that of TIMERN_BGLOAD. However,
     *   the timer counter does not restart counting. After the counter
     *   decreases to 0, the value of TIMERN_LOAD (that is,
     *   the value of TIMERN_BGLOAD) is reloaded to the counter.
     *
     * In other words, for periodic mode, load BGLOAD first, then write to
     * LOAD. For oneshot mode, only write to LOAD. For good measure, write 0
     * to BGLOAD.
     */
    regs->bgload = is_periodic ? ticks : 0;
    regs->load = ticks;

    /* The TIMERN_VALUE register is read-only. */
    regs->control = TCLR_STARTTIMER | TCLR_VALUE_32
                    | (is_periodic ? TCLR_AUTORELOAD : TCLR_ONESHOT)
                    | (enable_intr ? TCLR_INTENABLE : 0);
}
