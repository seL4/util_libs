/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/fdt.h>
#include <platsupport/ltimer.h>
#include <platsupport/timer.h>

typedef struct {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* are we timeout or timestamp? */

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
void rk_destroy(rk_t *rk);
