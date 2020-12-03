/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/timer.h>

#pragma once

#define BUS_ADDR_OFFSET          0x7E000000
#define PADDDR_OFFSET            0xFE000000

/*
 * According to BCM2711 TRM section 10.1.:
 *      0x7e003000
 */
#define SYSTEM_TIMER_BUSADDR     0x7E003000
#define SYSTEM_TIMER_PADDR       (SYSTEM_TIMER_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)

/*
 * The system timer frequency is fixed at 1 MHz:
 *      in seL4/tools/dts/rpi4.dts under /soc/timer@7e003000
 */
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
 * BCM2711 TRM
 * section 6.2.4. VideoCore interrupts:             0,1,2,3
 * section 6.3.   GIC-400 - VC peripheral IRQs:     96
 * => System Timer IRQs (4 different channels):
 *      96 + {0,1,2,3} = {96,97,98,99}
 */
#define SYSTEM_TIMER_MATCH_IRQ_START   0x60
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
