/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_BSD)
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

/* Memory maps */
#ifdef CONFIG_PLAT_ZYNQMP
#define TTC0_PADDR               0xFF110000
#define TTC1_PADDR               0xFF120000
#define TTC2_PADDR               0xFF130000
#define TTC3_PADDR               0xFF140000
#else
#define TTC0_PADDR               0xF8001000
#define TTC1_PADDR               0xF8002000
#endif

#define IRQS_PER_TTC 3

#define TTC_TIMER_SIZE           0x1000
#define TTC0_TIMER_SIZE          TTC_TIMER_SIZE
#define TTC1_TIMER_SIZE          TTC_TIMER_SIZE
#ifdef CONFIG_PLAT_ZYNQMP
#define TTC2_TIMER_SIZE          TTC_TIMER_SIZE
#define TTC3_TIMER_SIZE          TTC_TIMER_SIZE
#endif

/* IRQs */
#ifdef CONFIG_PLAT_ZYNQMP
#define TTC0_TIMER1_IRQ          68
#define TTC0_TIMER2_IRQ          69
#define TTC0_TIMER3_IRQ          70
#define TTC1_TIMER1_IRQ          71
#define TTC1_TIMER2_IRQ          72
#define TTC1_TIMER3_IRQ          73
#define TTC2_TIMER1_IRQ          74
#define TTC2_TIMER2_IRQ          75
#define TTC2_TIMER3_IRQ          76
#define TTC3_TIMER1_IRQ          77
#define TTC3_TIMER2_IRQ          78
#define TTC3_TIMER3_IRQ          79
#define TTC0_PATH "/amba/timer@ff110000"
#define TTC1_PATH "/amba/timer@ff120000"
#define TTC2_PATH "/amba/timer@ff130000"
#define TTC3_PATH "/amba/timer@ff140000"
#else
#define TTC0_TIMER1_IRQ          42
#define TTC0_TIMER2_IRQ          43
#define TTC0_TIMER3_IRQ          44
#define TTC1_TIMER1_IRQ          69
#define TTC1_TIMER2_IRQ          70
#define TTC1_TIMER3_IRQ          71
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

static const uintptr_t zynq_timer_paddrs[] = {
    [TTC0_TIMER1] = TTC0_PADDR,
    [TTC0_TIMER2] = TTC0_PADDR,
    [TTC0_TIMER3] = TTC0_PADDR,
    [TTC1_TIMER1] = TTC1_PADDR,
    [TTC1_TIMER2] = TTC1_PADDR,
#ifndef CONFIG_PLAT_ZYNQMP
    [TTC1_TIMER3] = TTC1_PADDR
#else
    [TTC1_TIMER3] = TTC1_PADDR,
    [TTC2_TIMER1] = TTC2_PADDR,
    [TTC2_TIMER2] = TTC2_PADDR,
    [TTC2_TIMER3] = TTC2_PADDR,
    [TTC3_TIMER1] = TTC3_PADDR,
    [TTC3_TIMER2] = TTC3_PADDR,
    [TTC3_TIMER3] = TTC3_PADDR
#endif
};

static const int zynq_timer_irqs[] = {
    [TTC0_TIMER1] = TTC0_TIMER1_IRQ,
    [TTC0_TIMER2] = TTC0_TIMER2_IRQ,
    [TTC0_TIMER3] = TTC0_TIMER3_IRQ,
    [TTC1_TIMER1] = TTC1_TIMER1_IRQ,
    [TTC1_TIMER2] = TTC1_TIMER2_IRQ,
#ifndef CONFIG_PLAT_ZYNQMP
    [TTC1_TIMER3] = TTC1_TIMER3_IRQ
#else
    [TTC1_TIMER3] = TTC1_TIMER3_IRQ,
    [TTC2_TIMER1] = TTC2_TIMER1_IRQ,
    [TTC2_TIMER2] = TTC2_TIMER2_IRQ,
    [TTC2_TIMER3] = TTC2_TIMER3_IRQ,
    [TTC3_TIMER1] = TTC3_TIMER1_IRQ,
    [TTC3_TIMER2] = TTC3_TIMER2_IRQ,
    [TTC3_TIMER3] = TTC3_TIMER3_IRQ
#endif
};

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

static inline uintptr_t ttc_paddr(ttc_id_t id)
{
    if (id >= TTC0_TIMER1 && id < NTIMERS) {
        return zynq_timer_paddrs[id];
    } else {
        return 0;
    }
}

static inline int ttc_irq(ttc_id_t id)
{
    if (id >= TTC0_TIMER1 && id < NTIMERS) {
        return zynq_timer_irqs[id];
    } else {
        return 0;
    }
}

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
