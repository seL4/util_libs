/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#pragma once

#include <platsupport/fdt.h>
#include <platsupport/ltimer.h>
#include <platsupport/timer.h>

#define RK_TIMER_PATH "/timer@ff6b0000"

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
    uint32_t load_count0; /* 0 */
    uint32_t load_count1; /* 4 */
    uint32_t current_value0; /* 8 */
    uint32_t current_value1; /* C */
    uint32_t control_register; /*10 */
    uint32_t pad0;/* 14 */
    uint32_t interrupt_status; /* 18 */
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
    irq_id_t irq_id;
} rk_t;

typedef struct {
    char *fdt_path;
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
