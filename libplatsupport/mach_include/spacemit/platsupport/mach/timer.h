/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

/* The K1 SoC has multiple timers, the 'Timer1' peripheral is used
 * by this driver. Each timer peripheral consists of three individual
 * counters/timers, confusingly named Timer 0, Timer 1 and Timer 2.
 * Below Timer 0 is called TIMER0 and is used as a counter and Timer 1
 * is called TIMER1 and used for timeouts.
 * K1 SoC spec : https://developer.spacemit.com/documentation?token=AZsXwVrqaisaDGkUqMWcNuMVnPb
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SPACEMIT_TIMER_BASE      0xD4014000
#define APB_CLK_BASE             0xD4015000
#define SPACEMIT_TIMER_SIZE      0x2000u
#define SPACEMIT_TIMER0_IRQ      23u // timer0_1_irq: Timer 0 IRQ of Timer1
#define SPACEMIT_TIMER1_IRQ      24u // timer0_2_irq: Timer 0 IRQ of Timer2
#define SLOW_CLOCK false

#if SLOW_CLOCK
#define SPACEMIT_TIMER_TICKS_PER_SECOND  32768u
#define K1_TCCR_CS0_VALUE                0x1u   /* slow clock */
#else
#define SPACEMIT_TIMER_TICKS_PER_SECOND  12800000u
#define K1_TCCR_CS0_VALUE                0x0u   /* fast clock */
#endif

#define SPACEMIT_NUM_TIMERS 3
#define SPACEMIT_TIMER_MAX_TICKS UINT32_MAX
#define APBC_TIMERx_CLK_RST_OFFSET  0x34u
#define SPACEMIT_TIMERS_CNT_OFFSET  0x90u

typedef struct {
    uint32_t tcer;         // Timer Count Enable Register
    uint32_t tcmr;         // Timer Count Mode Register
    uint32_t tcrr;         // Timer Count Restart Register
    uint32_t tccr;         // Timer Clock Control Register

    uint32_t tmr[3][4];    // Timer Match Register (TMRx)
    uint32_t tplvr[4];     // Timer Preload Value Register (TPLVRx)
    uint32_t tplcr[4];     // Timer Preload Control Register (TPLCRx)

    uint32_t tier[4];      // Timer Interrupt Enable Register (TIERx)
    uint32_t ticr[4];      // Timer Interrupt Clear Register (TICLRx)
    uint32_t tsr[4];       // Timer Status Register (TSRx)

    uint32_t tcnt[4];      // Timer Count Register (TCRx)
} spacemit_timer_regs_t;
static_assert(offsetof(spacemit_timer_regs_t, tcnt) == SPACEMIT_TIMERS_CNT_OFFSET,
              "struct spacemit_timer_regs_t has incorrect layout");

typedef struct {
    volatile spacemit_timer_regs_t *regs;
    uint8_t  timer_n;
    uint32_t value_h;
    uint32_t *vaddr;
} spacemit_timer_t;

void spacemit_timer_enable(spacemit_timer_t *timer);
void spacemit_timer_disable(spacemit_timer_t *timer);
uint64_t spacemit_timer_get_time(spacemit_timer_t *timer);
void spacemit_timer_reset(spacemit_timer_t *timer);
int spacemit_timer_set_timeout(spacemit_timer_t *timer, uint64_t ns, bool is_periodic);
void spacemit_timer_disable_all(void *vaddr);
void spacemit_timer_init(spacemit_timer_t *timer, void *vaddr, uint64_t channel);
