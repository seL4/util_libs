/*
 * Copyright 2020, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/ltimer.h>
#include <platsupport/timer.h>
#include <platsupport/pmem.h>
#include <platsupport/plat/icicle_mss.h>
#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * The Polarfire SoC has two 32 bit decrementing counters
 */

#define MSTIMER_BASE_PADDR 0x20125000
#define MSTIMER1_PADDR     0x20125000
#define MSTIMER2_PADDR     0x20125018
#define MSTIMER1_OFFSET    (MSTIMER1_PADDR - MSTIMER_BASE_PADDR)
#define MSTIMER2_OFFSET    (MSTIMER2_PADDR - MSTIMER_BASE_PADDR)


#define MSTIMER_TIMER1INT 82
#define MSTIMER_TIMER2INT 83

#define CONTROL_INTEN 2
#define CONTROL_MODE  1
#define CONTROL_ENABLE 0

#define MS_TIMER_INT (1<<CONTROL_INTEN)

// Control Mode:
//  - Set: Oneshot
//  - Clear: Periodic
#define MS_TIMER_MODE         (1<<CONTROL_MODE)

// Timer Enable
//  - Set: Enabled
//  - Clear: Disabled
#define MS_TIMER (1<<CONTROL_ENABLE)

#define MS_TIMER64_CONTROL_REG 18
#define MS_TIMER64_MODE_REG 21

struct mstimer_map {
    uint32_t value;
    uint32_t load_value;
    uint32_t bg_load_value;
    uint32_t control;
    uint32_t raw_interrupt_status;
    uint32_t masked_interrupt_status;
};

typedef struct mstimer {
    volatile struct mstimer_map *register_map;
    uint32_t time_h;
    ps_irq_t irq;
    ps_io_ops_t ops;
} mstimer_t;

typedef struct {
    void *base_vaddr;
    uint64_t base_address_offset;
    ps_irq_t *irq;
} mstimer_config_t;

int mstimer_start(mstimer_t *mstimer);
int mstimer_stop(mstimer_t *mstimer);
void  mstimer_handle_irq(mstimer_t *mstimer, uint32_t irq);
uint64_t mstimer_get_time(mstimer_t *mstimer);
int mstimer_reset(mstimer_t *mstimer);
int mstimer_set_timeout(mstimer_t *mstimer, uint64_t ns, bool periodic);
int mstimer_init(mstimer_t *mstimer, ps_io_ops_t ops, mstimer_config_t mstimer_config);
