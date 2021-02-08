/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/timer.h>

#pragma once

#define BUS_ADDR_OFFSET          0x7E000000
#define PADDDR_OFFSET            0x3F000000

#define SYSTEM_TIMER_BUSADDR     0x7E003000
#define SYSTEM_TIMER_PADDR       (SYSTEM_TIMER_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)

/* The system timer frequency is fixed a 1 MHz */
#define SYSTEM_TIMER_FREQ        (1 * MHZ)
#define SYSTEM_TIMER_NS_PER_TICK (GHZ / SYSTEM_TIMER_FREQ)

/*
 * IRQs generated when timer matches occur.
 *
 * The system timer generates a different IRQ for each of the 4 match
 * registers whenever the low 32-bits of the counter match the
 * corresponding compare register.
 *
 * To clear an interrupt for a partiuclar match, simply write a 1 to the
 * the bit with an index corresponding to the match register.
 *
 * DO NOT USE compare 0 or 2, they overlap with GPU IRQs. (we use 1).
 */
#define SYSTEM_TIMER_MATCH_COUNT       0x04
#define SYSTEM_TIMER_MATCH             0x01

/*
 * The standard IRQs start at 64, and this timer uses the first 4 IRQs,
 * one for each match register.
 */
#define SYSTEM_TIMER_MATCH_IRQ_START   0x40
#define SYSTEM_TIMER_MATCH_IRQ(n)      (SYSTEM_TIMER_MATCH_IRQ_START + n)

typedef struct {
    /* Status control register. */
    uint32_t ctrl;
    /* Low 32 bits of the 64-bit free-running counter. */
    uint32_t counter_low;
    /* High 32 bits of the 64-bit free-running counter. */
    uint32_t counter_high;
    /* Four compare registers to trigger IRQs. */
    uint32_t compare[SYSTEM_TIMER_MATCH_COUNT];
} system_timer_regs_t;

typedef struct {
    system_timer_regs_t *regs;
} system_timer_t;

typedef struct {
    void *vaddr;
} system_timer_config_t;

int system_timer_init(system_timer_t *timer, system_timer_config_t config);

uint64_t system_timer_get_time(system_timer_t *timer);

int system_timer_set_timeout(system_timer_t *timer, uint64_t ns);

int system_timer_handle_irq(system_timer_t *timer);

int system_timer_reset(system_timer_t *timer);
