/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utils/util.h>
#include <inttypes.h>
#include <utils/fence.h>
#include <utils/arch/io.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define CLEANUP_FAIL_TEXT "Failed to cleanup the TTC after failing to initialise it"

#define CLKCTRL_EXT_NEDGE           BIT(6)
#define CLKCTRL_EXT_SRC_EN          BIT(5)
#define CLKCTRL_PRESCALE_VAL(N)     (((N) & 0xf) << 1) /* rate = clk_src/[2^(N+1)] */
#define CLKCTRL_GET_PRESCALE_VAL(v) (((v) >> 1) & 0xf)
#define CLKCTRL_PRESCALE_EN         BIT(0)
#define CLKCTRL_PRESCALE_MASK       (CLKCTRL_PRESCALE_VAL(0xf) | CLKCTRL_PRESCALE_EN)

/* Waveform polarity: When this bit is high, the
 * waveform output goes from high to low on
 * Match_1 interrupt and returns high on overflow
 * or interval interrupt; when low, the waveform
 * goes from low to high on Match_1 interrupt and
 * returns low on overflow or interval interrupt */
#define CNTCTRL_WAVE_POL BIT(6)
/* Output waveform enable, active low. */
#define CNTCTRL_WAVE_EN  BIT(5)
/* Setting this bit high resets the counter value and
 * restarts counting; the RST bit is automatically
 * cleared on restart. */
#define CNTCTRL_RST      BIT(4)
/* Register Match mode: when Match is set, an
 * interrupt is generated when the count value
 * matches one of the three match registers and the
 * corresponding bit is set in the Interrupt Enable
 * register.
 */
#define CNTCTRL_MATCH    BIT(3)
/* Register Match mode: when Match is set, an
 * interrupt is generated when the count value
 * matches one of the three match registers and the
 * corresponding bit is set in the Interrupt Enable
 * register. */
#define CNTCTRL_DECR     BIT(2)
/* When this bit is high, the timer is in Interval
 * Mode, and the counter generates interrupts at
 * regular intervals; when low, the timer is in
 * overflow mode. */
#define CNTCTRL_INT      BIT(1)
/* Disable counter: when this bit is high, the counter
 * is stopped, holding its last value until reset,
 *  restarted or enabled again. */
#define CNTCTRL_STOP     BIT(0)

/* Event timer overflow interrupt */
#define INT_EVENT_OVR    BIT(5)
/* Counter overflow */
#define INT_CNT_OVR      BIT(4)
/* Match 3 interrupt */
#define INT_MATCH2       BIT(3)
/* Match 2 interrupt */
#define INT_MATCH1       BIT(2)
/* Match 1 interrupt */
#define INT_MATCH0       BIT(1)
/* Interval interrupt */
#define INT_INTERVAL     BIT(0)

/* Event Control Timer register: controls the behavior of the internal counter */

/* Specifies how to handle overflow at the internal counter (during the counting phase
 * of the external pulse)
 *
 * - When 0: Overflow causes E_En to be 0 (see E_En bit description)
 * - When 1: Overflow causes the internal counter to wrap around and continues incrementing
 */
#define EVCTRL_OVR       BIT(2)
/* Specifies the counting phase of the external pulse */
#define EVCTRL_LO        BIT(1)
/* When 0, immediately resets the internal counter to 0, and stops incrementing*/
#define EVCTRL_EN        BIT(0)

#define PRESCALE_MAX       0xf
#define PCLK_FREQ          111110000U

#ifdef CONFIG_PLAT_ZYNQMP
#define CNT_WIDTH 32
#define CNT_MAX ((1ULL << CNT_WIDTH) - 1)
#else
#define CNT_WIDTH 16
#define CNT_MAX (BIT(CNT_WIDTH) - 1)
#endif


/* Byte offsets into a field of ttc_tmr_regs_t for each ttc */
#define TTCX_TIMER1_OFFSET 0x0
#define TTCX_TIMER2_OFFSET 0x4
#define TTCX_TIMER3_OFFSET 0x8

#define TTCX_TIMER1_IRQ_POS 0
#define TTCX_TIMER2_IRQ_POS 1
#define TTCX_TIMER3_IRQ_POS 2

struct ttc_tmr_regs {
    /* Controls prescaler, selects clock input, edge */
    uint32_t clk_ctrl[3];   /* +0x00 */
    /* Enables counter, sets mode of operation, sets up/down
     * counting, enables matching, enables waveform output */
    uint32_t cnt_ctrl[3];   /* +0x0C */
    /* Returns current counter value */
    uint32_t cnt_val[3];    /* +0x18 */
    /* Sets interval value - If interval is enabled, this is the maximum value
     * that the counter will count up to or down from */
    uint32_t interval[3];   /* +0x24 */
    /* Sets match values, total 3 */
    uint32_t match[3][3];   /* +0x30 */
    /* Shows current interrupt status */
    uint32_t int_sts[3];    /* +0x54 */
    /* Enable interrupts */
    uint32_t int_en[3];     /* +0x60 */
    /* Enable event timer, stop timer, sets phrase */
    uint32_t event_ctrl[3]; /* +0x6C */
    /* Shows width of external pulse */
    uint32_t event[3];      /* +0x78 */
};
typedef volatile struct ttc_tmr_regs ttc_tmr_regs_t;

static freq_t _ttc_clk_get_freq(clk_t *clk);
static freq_t _ttc_clk_set_freq(clk_t *clk, freq_t hz);
static void _ttc_clk_recal(clk_t *clk);
static clk_t *_ttc_clk_init(clk_t *clk);

static inline ttc_tmr_regs_t *ttc_get_regs(ttc_t *ttc)
{
    return ttc->regs;
}

static inline size_t ttc_get_timer_shift(ttc_id_t id)
{
    switch (id) {
    case TTC0_TIMER1:
    case TTC1_TIMER1:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER1:
    case TTC3_TIMER1:
#endif
        return TTCX_TIMER1_OFFSET;
    case TTC0_TIMER2:
    case TTC1_TIMER2:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER2:
    case TTC3_TIMER2:
#endif
        return TTCX_TIMER2_OFFSET;
    case TTC0_TIMER3:
    case TTC1_TIMER3:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER3:
    case TTC3_TIMER3:
#endif
        return TTCX_TIMER3_OFFSET;
    default:
        ZF_LOGF("Invalid ttc_id_t!");
        return 0;
    }
}

static inline unsigned ttc_get_irq_pos(ttc_id_t id)
{
    switch (id) {
    case TTC0_TIMER1:
    case TTC1_TIMER1:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER1:
    case TTC3_TIMER1:
#endif
        return TTCX_TIMER1_IRQ_POS;
    case TTC0_TIMER2:
    case TTC1_TIMER2:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER2:
    case TTC3_TIMER2:
#endif
        return TTCX_TIMER2_IRQ_POS;
    case TTC0_TIMER3:
    case TTC1_TIMER3:
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER3:
    case TTC3_TIMER3:
#endif
        return TTCX_TIMER3_IRQ_POS;
    default:
        ZF_LOGF("Invalid ttc_id_t!");
        return 0;
    }
}

static inline char *ttc_get_device_path(ttc_id_t id)
{
    switch (id) {
    case TTC0_TIMER1:
    case TTC0_TIMER2:
    case TTC0_TIMER3:
        return TTC0_PATH;
    case TTC1_TIMER1:
    case TTC1_TIMER2:
    case TTC1_TIMER3:
        return TTC1_PATH;
#ifdef CONFIG_PLAT_ZYNQMP
    case TTC2_TIMER1:
    case TTC2_TIMER2:
    case TTC2_TIMER3:
        return TTC2_PATH;
    case TTC3_TIMER1:
    case TTC3_TIMER2:
    case TTC3_TIMER3:
        return TTC3_PATH;
#endif /* CONFIG_PLAT_ZYNQMP */
    default:
        ZF_LOGF("Invalid ttc_id_t!");
        return NULL;
    }
}

/****************** Clocks ******************/

static ttc_t *ttc_clk_get_priv(clk_t *clk)
{
    return (ttc_t *)clk->priv;
}

/* FPGA PL Clocks */
static freq_t _ttc_clk_get_freq(clk_t *clk)
{
    ttc_t *ttc = ttc_clk_get_priv(clk);
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
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

static freq_t _ttc_clk_set_freq(clk_t *clk, freq_t hz)
{
    ttc_t *ttc = ttc_clk_get_priv(clk);
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
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

static void _ttc_clk_recal(clk_t *clk UNUSED)
{
    assert(0);
}

static clk_t *_ttc_clk_init(clk_t *clk)
{
    return clk;
}

static inline freq_t _ttc_get_freq(ttc_t *ttc)
{
    return ttc->freq;
}

static inline freq_t _ttc_set_freq(ttc_t *ttc, freq_t hz)
{
    ttc->freq = clk_set_freq(&ttc->clk, hz);
    return ttc->freq;
}

static inline bool _ttc_check_interrupt(ttc_t *ttc)
{
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    /* The int_sts register is being accessed through typedef ttc_tmr_regs_t
     * which is marked volatile, so the compiler will not elide this read.
     */
    uint32_t res = *regs->int_sts;
    /* There are no data dependencies within this function that imply that the
     * CPU cannot reorder this read in the pipeline. Use a CPU read barrier to
     * inform the CPU that it should stall reads until this read has completed.
     */
    THREAD_MEMORY_RELEASE();

    return !!res;
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
static inline int _ttc_set_freq_for_ns(ttc_t *ttc, uint64_t ns, uint64_t *interval)
{
    freq_t fin, f;
    uint64_t interval_value;

    /* Set the clock source frequency
     * 1 / (fin / max_cnt) > interval
     * fin < max_cnt / interval */
    f = freq_cycles_and_ns_to_hz(CNT_MAX, ns);
    fin = _ttc_set_freq(ttc, f);
    if (fin > f) {
        /* This happens when the requested time is so long that the clock can't
         * run slow enough. In this case, the clock driver reported the minimum
         * rate it can run at, and we can use that to calculate a maximum time.
         */
        ZF_LOGE("Timeout too big for timer, max %"PRIu64", got %"PRIu64"\n",
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

int ttc_start(ttc_t *ttc)
{
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    *regs->cnt_ctrl &= ~CNTCTRL_STOP;
    return 0;
}

int ttc_stop(ttc_t *ttc)
{
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    *regs->cnt_ctrl |= CNTCTRL_STOP;
    return 0;
}

void ttc_freerun(ttc_t *ttc)
{
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    *regs->cnt_ctrl = CNTCTRL_RST;
    *regs->int_en = INT_EVENT_OVR | INT_CNT_OVR;
}

/* Set up the ttc to fire an interrupt every ns nanoseconds.
 * The first such interrupt may arrive before ns nanoseconds
 * have passed since calling. */
static int _ttc_periodic(ttc_tmr_regs_t *regs, uint64_t interval)
{
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

static void ttc_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    ttc_t *ttc = data;
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);

    bool interrupt_pending = _ttc_check_interrupt(ttc);

    if (ttc->is_timestamp) {
        /* Check if we already updated the timestamp when reading the time,
         * the interrupt status register should be empty if we did */
        if (interrupt_pending) {
            ttc->hi_time += ttc_ticks_to_ns(ttc, CNT_MAX);
        }
    } else {
        /* The MATCH0 interrupt is used in oneshot mode. It is enabled when a
         * oneshot function is called, and disabled here so only one interrupt
         * is triggered per call. */
        *regs->int_en &= ~INT_MATCH0;
    }

    /* Acknowledge the interrupt and call the user callback if any */
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the interrupt from the TTC");
    if (ttc->user_callback) {
        if (ttc->is_timestamp) {
            ttc->user_callback(ttc->user_callback_token, LTIMER_OVERFLOW_EVENT);
        } else {
            ttc->user_callback(ttc->user_callback_token, LTIMER_TIMEOUT_EVENT);
        }
    }
}

uint64_t ttc_ticks_to_ns(ttc_t *ttc, uint32_t ticks)
{
    if (!ttc) {
        return 0;
    }
    uint32_t fin = _ttc_get_freq(ttc);
    return freq_cycles_and_hz_to_ns(ticks, fin);
}

uint64_t ttc_get_time(ttc_t *ttc)
{
    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    uint32_t cnt = *regs->cnt_val;
    bool interrupt_pending = _ttc_check_interrupt(ttc);
    /* Check if there is an interrupt pending, i.e. counter overflowed */
    if (interrupt_pending) {
        /* Re-read the counter again */
        cnt = *regs->cnt_val;
        /* Bump the hi_time counter now, as there may be latency in serving the interrupt */
        ttc->hi_time += ttc_ticks_to_ns(ttc, CNT_MAX);
    }
    uint32_t fin = _ttc_get_freq(ttc);
    return ttc->hi_time + freq_cycles_and_hz_to_ns(cnt, fin);
}

/* Set up the ttc to fire an interrupt ns nanoseconds after this
 * function is called. */
static int _ttc_oneshot_relative(ttc_tmr_regs_t *regs, uint64_t interval)
{

    /* In overflow mode the ttc will continuously count up to 0xffff and reset to 0.
     * The ttc will be programmed to interrupt when the counter reaches
     * current_time + interval, allowing the addition to wrap around (16 bits).
     */

#ifdef CONFIG_PLAT_ZYNQMP
    *regs->match[0] = (interval + *regs->cnt_val);
#else
    *regs->match[0] = (interval + *regs->cnt_val) % BIT(CNT_WIDTH);
#endif

    /* Overflow mode: Continuously count from 0 to 0xffff (this is a 16 bit ttc).
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

int ttc_set_timeout(ttc_t *ttc, uint64_t ns, bool periodic)
{
    if (ttc == NULL) {
        return EINVAL;
    }

    /* Program the clock and compute the interval value */
    uint64_t interval;
    int error = _ttc_set_freq_for_ns(ttc, ns, &interval);
    if (error) {
        return error;
    }

    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    if (periodic) {
        return _ttc_periodic(regs, interval);
    } else {
        return _ttc_oneshot_relative(regs, interval);
    }
}

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    assert(token != NULL);
    ttc_t *ttc = token;
    ttc->regs = ps_pmem_map(&ttc->io_ops, pmem, false, PS_MEM_NORMAL);
    if (ttc->regs == NULL) {
        ZF_LOGE("Failed to map in registers for the TTC");
        return EIO;
    }
    /* This sets the base of the ttc_tmr_regs_t pointer to
     * an offset into the ttc's mmio region such that
     * ((ttc_tmr_regs_t*)vaddr)->clk_ctrl
     * (and all other registers) refers to the address of the
     * register relevant for the specified ttc device. */
    ttc->regs += ttc_get_timer_shift(ttc->id);
    ttc->timer_pmem = pmem;
    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    assert(token != NULL);
    ttc_t *ttc = token;
    assert(num_irqs == IRQS_PER_TTC);
    /* Get the corresponding IRQ position and register the IRQ if it matches */
    unsigned irq_pos = ttc_get_irq_pos(ttc->id);
    if (irq_pos == curr_num) {
        ttc->irq_id = ps_irq_register(&ttc->io_ops.irq_ops, irq, ttc_handle_irq, ttc);
        if (ttc->irq_id < 0) {
            ZF_LOGE("Failed to register the IRQ for TTC timer %d", ttc->id);
            return EIO;
        }
    }
    return 0;
}

int ttc_init(ttc_t *ttc, ttc_config_t config)
{
    if (ttc == NULL) {
        ZF_LOGE("ttc is NULL");
        return EINVAL;
    }

    /* Initialise all the struct members */
    ttc->io_ops = config.io_ops;
    ttc->user_callback = config.user_callback;
    ttc->user_callback_token = config.user_callback_token;
    ttc->irq_id = PS_INVALID_IRQ_ID;
    ttc->is_timestamp = config.is_timestamp;
    ttc->id = config.id;

    char *device_path = ttc_get_device_path(config.id);

    /* Read the timer's path in the DTB */
    ps_fdt_cookie_t *cookie = NULL;
    int error = ps_fdt_read_path(&ttc->io_ops.io_fdt, &ttc->io_ops.malloc_ops, device_path, &cookie);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&ttc->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(ttc_destroy(ttc), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the registers and allocate them */
    error = ps_fdt_walk_registers(&ttc->io_ops.io_fdt, cookie, allocate_register_callback, ttc);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&ttc->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(ttc_destroy(ttc), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Walk the interrupts and allocate the corresponding interrupt for this timer */
    error = ps_fdt_walk_irqs(&ttc->io_ops.io_fdt, cookie, allocate_irq_callback, ttc);
    if (error) {
        ZF_LOGF_IF(ps_fdt_cleanup_cookie(&ttc->io_ops.malloc_ops, cookie), CLEANUP_FAIL_TEXT);
        ZF_LOGF_IF(ttc_destroy(ttc), CLEANUP_FAIL_TEXT);
        return ENODEV;
    }

    /* Configure clock source */
    memset(&ttc->clk, 0, sizeof(ttc->clk));
    ttc->clk.name = STRINGIFY(config.id);
    ttc->clk.get_freq = _ttc_clk_get_freq;
    ttc->clk.set_freq = _ttc_clk_set_freq;
    ttc->clk.recal = _ttc_clk_recal;
    ttc->clk.init = _ttc_clk_init;
    ttc->clk.priv = ttc;

    if (config.clk_src) {
        clk_register_child(config.clk_src, &ttc->clk);
    }
    ttc->freq = clk_get_freq(&ttc->clk);

    ttc_tmr_regs_t *regs = ttc_get_regs(ttc);
    *regs->int_en = 0;
    FORCE_READ(regs->int_sts); /* Clear on read */
    *regs->cnt_ctrl = CNTCTRL_RST | CNTCTRL_STOP | CNTCTRL_INT | CNTCTRL_MATCH;
    *regs->clk_ctrl = 0;
    *regs->int_en = INT_INTERVAL;
    *regs->interval = CNT_MAX;

    return 0;
}

int ttc_destroy(ttc_t *ttc)
{
    assert(ttc != NULL);

    if (ttc->regs) {
        ZF_LOGF_IF(ttc_stop(ttc), "Failed to stop the TTC before de-allocating it");
        ps_io_unmap(&ttc->io_ops.io_mapper, ttc->regs, (size_t) ttc->timer_pmem.length);
    }

    if (ttc->irq_id != PS_INVALID_IRQ_ID) {
        ZF_LOGF_IF(ps_irq_unregister(&ttc->io_ops.irq_ops, ttc->irq_id), "Failed to unregister IRQ");
    }

    return 0;
}
