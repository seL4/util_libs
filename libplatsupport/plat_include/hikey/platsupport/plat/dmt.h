/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>

#define DMT_PATH "/soc/timer@f8008000"

#define DMT_REG_CHOICE 0
#define DMT_IRQ_CHOICE 0

static UNUSED timer_properties_t dmtimer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef volatile struct dmt_regs {
    uint32_t load;
    uint32_t value;
    uint32_t control;
    uint32_t intclr;
    uint32_t ris;
    uint32_t mis;
    uint32_t bgload;
} dmt_regs_t;

typedef struct {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* what are we being used for? */

    /* set in fdt helper */
    volatile dmt_regs_t *dmt_map;
    void *dmt_map_base;
    pmem_region_t pmem;
    irq_id_t irq_id;

    /* set in setup */
    uint32_t time_h;
} dmt_t;

typedef struct {
    char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;
} dmt_config_t;

int dmt_init(dmt_t *dmt, ps_io_ops_t ops, dmt_config_t config);
int dmt_init_secondary(dmt_t *dmt, dmt_t *dmtp, ps_io_ops_t ops, dmt_config_t config);
/* convert between dmt ticks and ns */
uint64_t dmt_ticks_to_ns(uint64_t ticks);
/* return true if an overflow irq is pending */
bool dmt_is_irq_pending(dmt_t *dmt);
int dmt_set_timeout_ticks(dmt_t *dmt, uint32_t ticks, bool periodic, bool irqs);
/* set a timeout in nano seconds */
int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic, bool irqs);
int dmt_start(dmt_t *dmt);
int dmt_stop(dmt_t *dmt);
uint64_t dmt_get_time(dmt_t *dmt);
uint64_t dmt_get_ticks(dmt_t *dmt);
void dmt_destroy(dmt_t *dmt);
