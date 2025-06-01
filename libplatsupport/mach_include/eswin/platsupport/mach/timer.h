/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* These values correspond to the DTS node 'soc/timer@0x51840000' */
#define ESWIN_TIMER_BASE 0x51840000
/* The device has a size of 0x8000, but we only need 0x1000 for the driver. */
#define ESWIN_TIMER_SIZE 0x1000
#define ESWIN_TIMER_IRQ 0x159

/* This is 24MHz */
#define ESWIN_TIMER_TICKS_PER_SECOND 0x16e3600

#define ESWIN_TIMER_MAX_TICKS UINT32_MAX

#define ESWIN_NUM_TIMERS 8

#define ESWIN_TIMER_ENABLED 0x1
#define ESWIN_TIMER_DISABLED 0x0
#define ESWIN_TIMER_MODE_FREE_RUNNING 0x0
#define ESWIN_TIMER_MODE_USER_DEFINED (1 << 1)
#define ESWIN_TIMER_IRQ_UNMASK 0x0

typedef struct {
    uint32_t load_count;
    uint32_t value;
    uint32_t ctrl;
    uint32_t eoi;
    uint32_t int_status;
} eswin_timer_regs_t;

typedef struct {
    volatile eswin_timer_regs_t *regs;
    /*
     * Stores the number of times the continuous counter timer has elapsed and started over.
     * This allows us to count to a higher number than allowed by the hardware.
     */
    uint32_t value_h;
} eswin_timer_t;

void eswin_timer_enable(eswin_timer_t *timer);
void eswin_timer_disable(eswin_timer_t *timer);
void eswin_timer_handle_irq(eswin_timer_t *timer);
uint64_t eswin_timer_get_time(eswin_timer_t *timer);
void eswin_timer_reset(eswin_timer_t *timer);
int eswin_timer_set_timeout(eswin_timer_t *timer, uint64_t ns, bool is_periodic);
void eswin_timer_disable_all(void *vaddr);
void eswin_timer_init(eswin_timer_t *timer, void *vaddr, uint64_t channel);
