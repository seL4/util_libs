/*
 * Copyright 2017, DornerWorks
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#pragma once

#include <platsupport/io.h>
#include <platsupport/ltimer.h>
#include <platsupport/fdt.h>
#include <platsupport/timer.h>
#include <platsupport/clock.h>

#define IRQS_PER_TTC 3

#ifdef CONFIG_PLAT_ZYNQMP
#define TTC0_PATH "/amba/timer@ff110000"
#define TTC1_PATH "/amba/timer@ff120000"
#define TTC2_PATH "/amba/timer@ff130000"
#define TTC3_PATH "/amba/timer@ff140000"
#else
/* zynq7000 */
#define TTC0_PATH "/amba/timer@f8001000"
#define TTC1_PATH "/amba/timer@f8002000"
#endif /* CONFIG_PLAT_ZYNQMP */

/* Timers */
typedef enum {
    TTC0_TIMER1,
    TTC0_TIMER2,
    TTC0_TIMER3,
    TTC1_TIMER1,
    TTC1_TIMER2,
    TTC1_TIMER3,
#ifdef CONFIG_PLAT_ZYNQMP
    TTC2_TIMER1,
    TTC2_TIMER2,
    TTC2_TIMER3,
    TTC3_TIMER1,
    TTC3_TIMER2,
    TTC3_TIMER3,
#endif
    NTIMERS
} ttc_id_t;
#define TMR_DEFAULT TTC0_TIMER1

typedef struct {
    bool is_timestamp;
    ps_io_ops_t io_ops;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    char *device_path;
    clk_t *clk_src;
    ttc_id_t id;
} ttc_config_t;

typedef struct {
    void *regs;
    ps_io_ops_t io_ops;
    irq_id_t irq_id;
    pmem_region_t timer_pmem;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    bool is_timestamp;
    uint64_t hi_time;
    clk_t clk;
    freq_t freq;
    ttc_id_t id;
} ttc_t;

static UNUSED timer_properties_t ttc_properties = {
    .upcounter = true,
    .timeouts = true,
    .bit_width = 16,
    .irqs = 1,
    .relative_timeouts = true,
    .absolute_timeouts = true
};

int ttc_init(ttc_t *ttc, ttc_config_t config);
int ttc_destroy(ttc_t *ttc);
int ttc_start(ttc_t *ttc);
int ttc_stop(ttc_t *ttc);
int ttc_set_timeout(ttc_t *ttc, uint64_t ns, bool periodic);
/* set the ttc to 0 and start free running, where the timer will
 * continually increment and trigger irqs on each overflow and reload to 0 */
void ttc_freerun(ttc_t *tcc);
uint64_t ttc_get_time(ttc_t *ttc);
/* convert from a ticks value to ns for a configured ttc */
uint64_t ttc_ticks_to_ns(ttc_t *ttc, uint32_t ticks);
