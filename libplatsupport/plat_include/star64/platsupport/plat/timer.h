/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * The JH7110 SoC contains a timer with four 32-bit counters. Each one of these
 * counters is referred to as a "channel".
 * At the time of writing the SoC documentation is quite minimal for the timer device
 * and so most of this driver is based on the Linux patches [1]
 *
 * [1]: https://patchwork.kernel.org/project/linux-riscv/patch/20230627055313.252519-3-xingyu.wu@starfivetech.com/.
 */

/* Channel information */
#define STARFIVE_TIMER_NUM_CHANNELS 4
#define STARFIVE_TIMER_CHANNEL_REGISTERS_LEN_IN_BYTES 0x40
#define STARFIVE_TIMER_CHANNEL_0_IRQ 69
#define STARFIVE_TIMER_CHANNEL_1_IRQ 70
#define STARFIVE_TIMER_CHANNEL_2_IRQ 71
#define STARFIVE_TIMER_CHANNEL_3_IRQ 72

/* This information comes from the DTS file */
#define STARFIVE_TIMER_BASE 0x13050000
#define STARFIVE_TIMER_REGISTER_WINDOW_LEN_IN_BYTES 0x10000
/* This is 24MHz */
#define STARFIVE_TIMER_TICKS_PER_SECOND 0x16e3600

#define STARFIVE_TIMER_MAX_TICKS UINT32_MAX

/* Register value constants */
#define STARFIVE_TIMER_MODE_CONTINUOUS 0
#define STARFIVE_TIMER_MODE_SINGLE 1
#define STARFIVE_TIMER_DISABLED 0
#define STARFIVE_TIMER_ENABLED 1
#define STARFIVE_TIMER_INTERRUPT_UNMASKED 0
#define STARFIVE_TIMER_INTERRUPT_MASKED 1
#define STARFIVE_TIMER_INTCLR_BUSY BIT(1)

typedef struct {
    /* Registers */
    /* this register doesn't seem to do anything */
    uint32_t status;
    uint32_t ctrl;
    uint32_t load;
    uint32_t unknown1;
    uint32_t enable;
    uint32_t reload;
    uint32_t value;
    uint32_t unknown2;
    uint32_t intclr;
    uint32_t intmask;
} starfive_timer_regs_t;

typedef struct {
    volatile starfive_timer_regs_t *regs;
    /*
     * Stores the number of times the continuous counter timer has elapsed and started over.
     * This allows us to count to a higher number than allowed by the hardware.
     */
    uint32_t value_h;
} starfive_timer_t;

void starfive_timer_start(starfive_timer_t *timer);
void starfive_timer_stop(starfive_timer_t *timer);
void starfive_timer_handle_irq(starfive_timer_t *timer);
uint64_t starfive_timer_get_time(starfive_timer_t *timer);
void starfive_timer_reset(starfive_timer_t *timer);
int starfive_timer_set_timeout(starfive_timer_t *timer, uint64_t ns, bool is_periodic);
void starfive_timer_disable_all_channels(void *vaddr);
void starfive_timer_init(starfive_timer_t *timer, void *vaddr, uint64_t channel);
