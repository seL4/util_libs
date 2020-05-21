/*
 * Copyright 2017, Data61
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

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>

#define DMTIMER2_PATH "/ocp/timer@48040000"
#define DMTIMER3_PATH "/ocp/timer@48042000"

#define DMT_REG_CHOICE 0
#define DMT_IRQ_CHOICE 0

static UNUSED timer_properties_t dmt_properties = {
    .upcounter = false,
    .timeouts = true,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;
} dmt_config_t;

struct dmt_map {
    uint32_t tidr; // 00h TIDR Identification Register
    uint32_t padding1[3];
    uint32_t cfg; // 10h TIOCP_CFG Timer OCP Configuration Register
    uint32_t padding2[3];
    uint32_t tieoi; // 20h IRQ_EOI Timer IRQ End-Of-Interrupt Register
    uint32_t tisrr; // 24h IRQSTATUS_RAW Timer IRQSTATUS Raw Register
    uint32_t tisr; // 28h IRQSTATUS Timer IRQSTATUS Register
    uint32_t tier; // 2Ch IRQENABLE_SET Timer IRQENABLE Set Register
    uint32_t ticr; // 30h IRQENABLE_CLR Timer IRQENABLE Clear Register
    uint32_t twer; // 34h IRQWAKEEN Timer IRQ Wakeup Enable Register
    uint32_t tclr; // 38h TCLR Timer Control Register
    uint32_t tcrr; // 3Ch TCRR Timer Counter Register
    uint32_t tldr; // 40h TLDR Timer Load Register
    uint32_t ttgr; // 44h TTGR Timer Trigger Register
    uint32_t twps; // 48h TWPS Timer Write Posted Status Register
    uint32_t tmar; // 4Ch TMAR Timer Match Register
    uint32_t tcar1; // 50h TCAR1 Timer Capture Register
    uint32_t tsicr; // 54h TSICR Timer Synchronous Interface Control Register
    uint32_t tcar2; // 58h TCAR2 Timer Capture Register
};

typedef struct dmt {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* what are we being used for? */

    /* set in fdt helper */
    volatile struct dmt_map *hw;
    pmem_region_t pmem;
    irq_id_t irq_id;

    /* set in setup */
    uint32_t time_h;
} dmt_t;

int dmt_init(dmt_t *dmt, ps_io_ops_t ops, dmt_config_t config);
int dmt_start(dmt_t *dmt);
int dmt_stop(dmt_t *dmt);
/* configure a timeout */
int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic);
/* start the ticking timer */
int dmt_start_ticking_timer(dmt_t *dmt);
void dmt_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data);
/* return true if an overflow is pending */
bool dmt_pending_overflow(dmt_t *dmt);
/* return time */
uint64_t dmt_get_time(dmt_t *dmt);
void dmt_destroy(dmt_t *dmt);
