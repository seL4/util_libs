/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/fdt.h>
#include <platsupport/ltimer.h>
#include <platsupport/timer.h>

#define RK_TIMER_PATH "/rktimer@ff850000"

#define RK_REG_CHOICE 0
#define RK_IRQ_CHOICE 0

static UNUSED timer_properties_t rk_properties = {
    .upcounter = true,
    .timeouts = true,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

struct rk_map {
    uint32_t load_count0;
    uint32_t load_count1;
    uint32_t current_value0;
    uint32_t current_value1;
    uint32_t load_count2;
    uint32_t load_count3;
    uint32_t interrupt_status;
    uint32_t control_register;
};

typedef struct {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* what are we being used for? */

    /* set in fdt helper */
    volatile struct rk_map *hw;
    void *rk_map_base;
    pmem_region_t pmem;
    ps_irq_t irq;
    irq_id_t irq_id;
} rk_t;

typedef struct {
    const char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;
} rk_config_t;

int rk_init(rk_t *rk, ps_io_ops_t ops, rk_config_t config);
int rk_init_secondary(rk_t *rk, rk_t *rkp, ps_io_ops_t ops, rk_config_t config);
int rk_start_timestamp_timer(rk_t *rk);
int rk_stop(rk_t *rk);
uint64_t rk_get_time(rk_t *rk);
int rk_set_timeout(rk_t *rk, uint64_t ns, bool periodic);
/* return true if a match is pending */
bool rk_pending_match(rk_t *rk);
void rk_destroy(rk_t *rk);
