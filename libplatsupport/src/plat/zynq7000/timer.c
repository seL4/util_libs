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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>


#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define CLKCTRL_EXT_NEDGE           BIT(6)
#define CLKCTRL_EXT_SRC_EN          BIT(5)
#define CLKCTRL_PRESCALE_VAL(N)     (((N) & 0xf) << 1) /* rate = clk_src/[2^(N+1)] */
#define CLKCTRL_GET_PRESCALE_VAL(v) (((v) >> 1) & 0xf)
#define CLKCTRL_PRESCALE_EN         BIT(0)
#define CLKCTRL_PRESCALE_MASK       (CLKCTRL_PRESCALE_VAL(0xf) | CLKCTRL_PRESCALE_EN)

#define CNTCTRL_WAVE_POL BIT(6)
#define CNTCTRL_WAVE_EN  BIT(5)
#define CNTCTRL_RST      BIT(4)
#define CNTCTRL_MATCH    BIT(3)
#define CNTCTRL_DECR     BIT(2)
#define CNTCTRL_INT      BIT(1)
#define CNTCTRL_STOP     BIT(0)

#define INT_EVENT_OVR    BIT(5)
#define INT_CNT_OVR      BIT(4)
#define INT_MATCH2       BIT(3)
#define INT_MATCH1       BIT(2)
#define INT_MATCH0       BIT(1)
#define INT_INTERVAL     BIT(0)

#define EVCTRL_OVR       BIT(2)
#define EVCTRL_LO        BIT(1)
#define EVCTRL_EN        BIT(0)

#define PRESCALE_MAX       0xf
#define PCLK_FREQ          111110000U

#define CNT_WIDTH 16
#define CNT_MAX (BIT(CNT_WIDTH) - 1)

#define TTC_CLK_DATA(id) {              \
        .name = #id,                    \
        .req_freq = 0,                  \
        .parent = NULL,                 \
        .sibling = NULL,                \
        .child = NULL,                  \
        .clk_sys = NULL,                \
        .get_freq = _ttc_clk_get_freq,  \
        .set_freq = _ttc_clk_set_freq,  \
        .recal = _ttc_clk_recal,        \
        .init = _ttc_clk_init,          \
        .priv = (void*)&_timers[id]     \
    }

/* Byte offsets into a field of ttc_tmr_regs_t for each timer */
#define TTCX_TIMER1_OFFSET 0x0
#define TTCX_TIMER2_OFFSET 0x4
#define TTCX_TIMER3_OFFSET 0x8

struct ttc_tmr_regs {
    uint32_t clk_ctrl[3];   /* +0x00 */
    uint32_t cnt_ctrl[3];   /* +0x0C */
    uint32_t cnt_val[3];    /* +0x18 */
    uint32_t interval[3];   /* +0x24 */
    uint32_t match[3][3];   /* +0x30 */
    uint32_t int_sts[3];    /* +0x54 */
    uint32_t int_en[3];     /* +0x60 */
    uint32_t event_ctrl[3]; /* +0x6C */
    uint32_t event[3];      /* +0x78 */
};
typedef volatile struct ttc_tmr_regs ttc_tmr_regs_t;

struct ttc_data {
    clk_t clk;
    freq_t freq;
    ttc_tmr_regs_t* regs;
};

static freq_t _ttc_clk_get_freq(clk_t* clk);
static freq_t _ttc_clk_set_freq(clk_t* clk, freq_t hz);
static void _ttc_clk_recal(clk_t* clk);
static clk_t* _ttc_clk_init(clk_t* clk);


static pstimer_t _timers[NTIMERS];
static struct ttc_data _timer_data[NTIMERS] = {
    [TTC0_TIMER1] = { .clk = TTC_CLK_DATA(TTC0_TIMER1), .freq = 0, .regs = NULL },
    [TTC0_TIMER2] = { .clk = TTC_CLK_DATA(TTC0_TIMER2), .freq = 0, .regs = NULL },
    [TTC0_TIMER3] = { .clk = TTC_CLK_DATA(TTC0_TIMER3), .freq = 0, .regs = NULL },
    [TTC1_TIMER1] = { .clk = TTC_CLK_DATA(TTC1_TIMER1), .freq = 0, .regs = NULL },
    [TTC1_TIMER2] = { .clk = TTC_CLK_DATA(TTC1_TIMER2), .freq = 0, .regs = NULL },
    [TTC1_TIMER3] = { .clk = TTC_CLK_DATA(TTC1_TIMER3), .freq = 0, .regs = NULL },
};

static inline enum timer_id
timer_get_id(const pstimer_t *timer)
{
    return timer - _timers;
}

static inline struct ttc_data*
timer_get_data(const pstimer_t *timer) {
    return (struct ttc_data*)timer->data;
}

static inline ttc_tmr_regs_t*
timer_get_regs(const pstimer_t *timer)
{
    return timer_get_data(timer)->regs;
}

/****************** Clocks ******************/

static pstimer_t*
ttc_clk_get_priv(clk_t* clk)
{
    return (pstimer_t*)clk->priv;
}

/* FPGA PL Clocks */
static freq_t
_ttc_clk_get_freq(clk_t* clk)
{
    pstimer_t* timer = ttc_clk_get_priv(clk);
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint32_t clk_ctrl;
    freq_t fin, fout;
    /* Get the parent frequency */
    if (clk->parent) {
        fin = clk_get_freq(clk->parent);
    } else {
        fin = PCLK_FREQ;
    }
    /* Calculate fout */
    clk_ctrl = *regs->clk_ctrl;
    if (clk_ctrl & CLKCTRL_PRESCALE_EN) {
        fout = fin >> (CLKCTRL_GET_PRESCALE_VAL(clk_ctrl) + 1);
    } else {
        fout = fin;
    }
    /* Return */
    return fout;
}

static freq_t
_ttc_clk_set_freq(clk_t* clk, freq_t hz)
{
    pstimer_t* timer = ttc_clk_get_priv(clk);
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint32_t v;
    freq_t fin;
    int ps;
    /* Determine input clock frequency */
    if (clk->parent) {
        fin = clk_get_freq(clk->parent);
    } else {
        fin = PCLK_FREQ;
    }
    /* Find a prescale value */
    for (ps = 0; fin > hz; ps++, fin >>= 1);
    if (ps > PRESCALE_MAX) {
        return 0;
    }
    /* Configure the timer */
    v = regs->clk_ctrl[0] & ~CLKCTRL_PRESCALE_MASK;
    if (ps > 0) {
        v |= CLKCTRL_PRESCALE_EN | CLKCTRL_PRESCALE_VAL(ps - 1);
    } else {
        v &= ~CLKCTRL_PRESCALE_EN;
    }
    *regs->clk_ctrl = v;
    return clk_get_freq(clk);
}

static void
_ttc_clk_recal(clk_t* clk UNUSED)
{
    assert(0);
}

static clk_t*
_ttc_clk_init(clk_t* clk)
{
    return clk;
}

static inline freq_t
_ttc_get_freq(const pstimer_t* timer)
{
    struct ttc_data* data = timer_get_data(timer);
    return data->freq;
}

static inline freq_t
_ttc_set_freq(const pstimer_t* timer, freq_t hz)
{
    struct ttc_data* data = timer_get_data(timer);
    data->freq = clk_set_freq(&data->clk, hz);
    return data->freq;
}

/********************************************/

/* Computes the optimal clock frequency for interrupting after
 * a given period of time. This will be the highest frequency
 * such that starting from 0, the timer counter will reach its
 * maximum value in AT MOST the specified time.
 *
 * If no such frequency is supported by the clock (ie. the
 * requested time is too high) this returns ETIME. Returns 0
 * on success.
 *
 * If a frequency is found, the clock is reprogrammed to run
 * at that frequency. The number of ticks it will take for the
 * requested time to pass (ie. the interval) is computed and
 * returned via an argument (interval). */
static inline int
_ttc_set_freq_for_ns(const pstimer_t *timer, uint64_t ns, uint64_t *interval)
{
    freq_t fin, f;
    uint64_t interval_value;

    /* Set the clock source frequency
     * 1 / (fin / max_cnt) > interval
     * fin < max_cnt / interval */
    f = freq_cycles_and_ns_to_hz(CNT_MAX, ns);
    fin = _ttc_set_freq(timer, f);
    if (fin > f) {
        /* This happens when the requested time is so long that the clock can't
         * run slow enough. In this case, the clock driver reported the minimum
         * rate it can run at, and we can use that to calculate a maximum time.
         */
        ZF_LOGE("Timeout too big for timer, max %llu, got %llu\n",
                            freq_cycles_and_hz_to_ns(CNT_MAX, fin), ns);

        return ETIME;
    }

    interval_value = freq_ns_and_hz_to_cycles(ns, fin);

    assert(interval_value <= CNT_MAX);

    if (interval) {
        *interval = interval_value;
    }

    return 0;
}

static uint32_t
_ttc_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    int id = timer_get_id(timer);
    return zynq_timer_irqs[id];
}

static int
_ttc_timer_start(const pstimer_t *timer)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    *regs->cnt_ctrl &= ~CNTCTRL_STOP;
    return 0;
}

static int
_ttc_timer_stop(const pstimer_t *timer)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    *regs->cnt_ctrl |= CNTCTRL_STOP;
    return 0;
}

static int
_ttc_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    /* As this is a 16 bit timer, it overflows too frequently
     * for setting absolute timeouts to be useful. */
    return ENOSYS;
}

/* Set up the timer to fire an interrupt every ns nanoseconds.
 * The first such interrupt may arrive before ns nanoseconds
 * have passed since calling. */
static int
_ttc_periodic(const pstimer_t *timer, uint64_t ns)
{
    uint64_t interval;

    /* Program the clock and compute the interval value */
    int error = _ttc_set_freq_for_ns(timer, ns, &interval);
    if (error) {
        return error;
    }


    ttc_tmr_regs_t* regs = timer_get_regs(timer);

    *regs->interval = interval;

    /* Interval mode: Continuously count from 0 to value in interval register,
     * triggering an interval interrupt and resetting the counter to 0
     * whenever the counter passes through 0. */
    *regs->cnt_ctrl |= CNTCTRL_INT;

    /* The INTERVAL interrupt is used in periodic mode. The only source of
     * interrupts will be when the counter passes through 0 after reaching
     * the value in the interval register. */
    *regs->int_en = INT_INTERVAL;

    return 0;
}

static void
_ttc_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);

    /* The MATCH0 interrupt is used in oneshot mode. It is enabled when a
     * oneshot function is called, and disabled here so only one interrupt
     * is triggered per call. */
    *regs->int_en &= ~INT_MATCH0;

    FORCE_READ(regs->int_sts); /* Clear on read */
}

static uint64_t
_ttc_get_time(const pstimer_t *timer)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint32_t cnt = *regs->cnt_val;
    uint32_t fin = _ttc_get_freq(timer);
    return freq_cycles_and_hz_to_ns(cnt, fin);
}

/* Set up the timer to fire an interrupt ns nanoseconds after this
 * function is called. */
static int
_ttc_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    uint64_t interval;

    /* Program the clock and compute the interval value */
    int error = _ttc_set_freq_for_ns(timer, ns, &interval);
    if (error) {
        return error;
    }

    ttc_tmr_regs_t* regs = timer_get_regs(timer);

    /* In overflow mode the timer will continuously count up to 0xffff and reset to 0.
     * The timer will be programmed to interrupt when the counter reaches
     * current_time + interval, allowing the addition to wrap around (16 bits).
     */

    *regs->match[0] = (interval + *regs->cnt_val) % BIT(CNT_WIDTH);

    /* Overflow mode: Continuously count from 0 to 0xffff (this is a 16 bit timer).
     * In this mode no interrval interrupts. A match interrupt (MATCH0) will be used
     * in this mode. */
    *regs->cnt_ctrl &= ~CNTCTRL_INT;

    /* The MATCH0 interrupt is used in oneshot mode. The only source of interrupts
     * will be when the counter passes through the value in the match[0] register.
     * This interrupt is disabled in the irq handler so it is only triggered once.
     */
    *regs->int_en = INT_MATCH0;

    return 0;
}

pstimer_t *
ps_get_timer(enum timer_id id, timer_config_t *config)
{
    ttc_tmr_regs_t* regs;
    pstimer_t *timer;
    clk_t* clk;
    struct ttc_data *timer_data;
    void* vaddr;

    /* This sets the base of the ttc_tmr_regs_t pointer to
     * an offset into the timer's mmio region such that
     * ((ttc_tmr_regs_t*)vaddr)->clk_ctrl
     * (and all other registers) refers to the address of the
     * register relevant for the specified timer device. */
    vaddr = config->vaddr;
    switch (id) {
    case TTC0_TIMER1:
    case TTC1_TIMER1:
        vaddr += TTCX_TIMER1_OFFSET;
        break;
    case TTC0_TIMER2:
    case TTC1_TIMER2:
        vaddr += TTCX_TIMER2_OFFSET;
        break;
    case TTC0_TIMER3:
    case TTC1_TIMER3:
        vaddr += TTCX_TIMER3_OFFSET;
        break;
    default:
        return NULL;
    }

    timer = &_timers[id];
    timer_data = &_timer_data[id];
    timer->data = timer_data;
    timer_data->regs = vaddr;

    /* Configure clock source */
    clk = &timer_data->clk;
    if (config->clk_src) {
        clk_register_child(config->clk_src, clk);
    }
    timer_data->freq = clk_get_freq(clk);

    timer->properties.upcounter = true;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 16;
    timer->properties.irqs = 1;
    timer->properties.absolute_timeouts = false;
    timer->properties.relative_timeouts = true;
    timer->properties.periodic_timeouts = true;

    timer->start = _ttc_timer_start;
    timer->stop = _ttc_timer_stop;
    timer->get_time = _ttc_get_time;
    timer->oneshot_absolute = _ttc_oneshot_absolute;
    timer->oneshot_relative = _ttc_oneshot_relative;
    timer->periodic = _ttc_periodic;
    timer->handle_irq = _ttc_handle_irq;
    timer->get_nth_irq = _ttc_get_nth_irq;

    regs = timer_get_regs(timer);
    *regs->int_en = 0;
    FORCE_READ(regs->int_sts); /* Clear on read */
    *regs->cnt_ctrl = CNTCTRL_RST | CNTCTRL_STOP | CNTCTRL_INT | CNTCTRL_MATCH;
    *regs->clk_ctrl = 0;
    *regs->int_en = INT_INTERVAL;
    *regs->interval = CNT_MAX;

    return timer;
}


pstimer_t*
ps_init_timer(int _id, ps_io_ops_t* io_ops)
{
    enum timer_id id = (enum timer_id)_id;
    timer_config_t tc;
    pstimer_t *timer;
    uintptr_t paddr;

    /* Find the physical address */
    if (id < 0 || id >= NTIMERS) {
        return NULL;
    }
    paddr = zynq_timer_paddrs[id];
    /* Map in the timer */
    tc.vaddr = ps_io_map(&io_ops->io_mapper, paddr, TTC_TIMER_SIZE, 0, PS_MEM_NORMAL);
    if (tc.vaddr == NULL) {
        return NULL;
    }
    /* Grab the default clock source */
    if (clock_sys_valid(&io_ops->clock_sys)) {
        tc.clk_src = clk_get_clock(&io_ops->clock_sys, CLK_CPU_1X);
    } else {
        tc.clk_src = NULL;
    }
    /* Initialise the timer */
    timer = ps_get_timer(id, &tc);
    if (timer == NULL) {
        return NULL;
    }
    return timer;
}
