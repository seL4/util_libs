/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/pmem.h>

/* System timer has four channels, however two of them (0 and 2) are used by
 * VideoCore
 */
#define BCM_TIMER_NUM_CHANNELS (4)

static UNUSED timer_properties_t bcm_timer_properties = {
    .upcounter = true,
    .bit_width = 64,
    .irqs = 1,
    .periodic_timeouts = true,
    .relative_timeouts = true,
    .absolute_timeouts = true,
    .timeouts = true,
};

typedef struct {
    uint32_t control;                           // control/status register
    uint32_t counter_low;                       // counter lower 32-bits
    uint32_t counter_high;                      // counter higher 32-bits
    uint32_t compare[BCM_TIMER_NUM_CHANNELS];   // compare registers
} bcm_system_timer_registers_t;

typedef struct {
    uint32_t frequency;
    uint32_t period;
    uint32_t channel;
    volatile bcm_system_timer_registers_t *registers;
    ps_io_ops_t ops;
    pmem_region_t pmem;
    irq_id_t irq;
    ltimer_callback_fn_t callback;
    void *callback_token;
} bcm_system_timer_t;

typedef struct {
    uint32_t channel;
    uint32_t frequency;
    char *fdt_path;
    uint32_t fdt_reg_choice;
    uint32_t fdt_irq_choice;
} bcm_system_timer_config_t;

int bcm_system_timer_init(bcm_system_timer_t *timer,
                          ps_io_ops_t ops,
                          ltimer_callback_fn_t callback,
                          void *callback_token,
                          bcm_system_timer_config_t config);
void bcm_system_timer_destroy(bcm_system_timer_t *timer);
int bcm_system_timer_reset(bcm_system_timer_t *timer);
int bcm_system_timer_get_time(bcm_system_timer_t *timer, uint64_t *time);
int bcm_system_timer_set_timeout(bcm_system_timer_t *timer,
                                 uint64_t ns,
                                 timeout_type_t type);
