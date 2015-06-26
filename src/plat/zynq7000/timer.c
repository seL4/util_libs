/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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
#define PCLK_FREQ          100000000U

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


static pstimer_t timers[NTIMERS];

static inline ttc_tmr_regs_t*
timer_get_regs(const pstimer_t *timer)
{
    return (ttc_tmr_regs_t*)timer->data;
}

static inline uint64_t _ttc_get_freq(const pstimer_t *timer)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint32_t clk_ctrl = *regs->clk_ctrl;
    if (clk_ctrl & CLKCTRL_PRESCALE_EN) {
        return PCLK_FREQ >> (CLKCTRL_GET_PRESCALE_VAL(clk_ctrl) + 1);
    } else {
        return PCLK_FREQ;
    }
}

static inline int _ttc_set_interval(const pstimer_t *timer, uint32_t ns)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint64_t interval;
    uint32_t ps;
    /* Choose a prescale value */
    interval = (uint64_t)ns * PCLK_FREQ / 1e9;
    ps = 0;
    while (interval >= BIT(16) && ps <= PRESCALE_MAX) {
        ps++;
        interval >>= 1;
    }
    /* Configure the timer */
    if (ps <= PRESCALE_MAX) {
        uint32_t clk_ctrl = *regs->clk_ctrl;
        clk_ctrl &= ~CLKCTRL_PRESCALE_MASK;
        if (ps) {
            clk_ctrl |= CLKCTRL_PRESCALE_EN | CLKCTRL_PRESCALE_VAL(ps - 1);
        }
        *regs->clk_ctrl = clk_ctrl;
        *regs->interval = interval;
        return 0;
    } else {
        return -1;
    }
}

static uint32_t
_ttc_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    int id = timer - timers;
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
    return _ttc_set_interval(timer, ns);
}

static int
_ttc_periodic(const pstimer_t *timer, uint64_t ns)
{
    return _ttc_set_interval(timer, ns);
}

static void
_ttc_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    (void)*regs->int_sts; /* Clear on read */
}

static uint64_t
_ttc_get_time(const pstimer_t *timer)
{
    ttc_tmr_regs_t* regs = timer_get_regs(timer);
    uint32_t cnt = *regs->cnt_val;
    uint32_t fin = _ttc_get_freq(timer);
    return (1e9 * cnt) / fin;
}

static int
_ttc_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    return _ttc_oneshot_absolute(timer, _ttc_get_time(timer) + ns);
}

pstimer_t *
ps_get_timer(enum timer_id id, timer_config_t *config)
{
    ttc_tmr_regs_t* regs;
    pstimer_t *timer;
    void* vaddr;

    vaddr = config->vaddr;
    switch (id) {
    case TTC0_TIMER1:
    case TTC1_TIMER1:
        vaddr += 0;
        break;
    case TTC0_TIMER2:
    case TTC1_TIMER2:
        vaddr += 4;
        break;
    case TTC0_TIMER3:
    case TTC1_TIMER3:
        vaddr += 8;
        break;
    default:
        return NULL;
    }

    timer = &timers[id];
    timer->data = vaddr;

    timer->properties.upcounter = true;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 16;
    timer->properties.irqs = 1;

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
    (void)*regs->int_sts; /* Clear on read */
    *regs->cnt_ctrl = CNTCTRL_RST | CNTCTRL_STOP | CNTCTRL_INT;
    *regs->clk_ctrl = 0;
    *regs->int_en = INT_INTERVAL;
    *regs->interval = 0xffff;

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
    /* Initialise the timer */
    timer = ps_get_timer(id, &tc);
    if (timer == NULL) {
        return NULL;
    }
    return timer;
}

