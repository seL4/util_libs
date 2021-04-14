/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* @AUTHOR(akroh@ertos.nicta.com.au) */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/plat/timer.h>

#define XOTMR_FIN                19200000UL
#define SLPTMR_FIN                  32768UL
static inline uint64_t get_tcxo_hz(void)
{
    return 7 * 1000 * 1000;
}

static inline uint64_t get_xo_hz(void)
{
    return XOTMR_FIN;
}

static inline uint64_t get_slp_hz(void)
{
    return SLPTMR_FIN;
}

/** Timer map offsets **/
#define KPSSSECURE_OFFSET               0x000
#define KPSSGPT0_OFFSET                 0x004
#define KPSSGPT1_OFFSET                 0x014
#define KPSSDGT_OFFSET                  0x024
#define KPSSWDT0_OFFSET                 0x038
#define KPSSWDT1_OFFSET                 0x060
#define KPSSSTAT_OFFSET                 0x088

#define GSSSECURE_OFFSET                KPSSSECURE_OFFSET
#define GSSGPT0_OFFSET                  KPSSGPT0_OFFSET
#define GSSGPT1_OFFSET                  KPSSGPT1_OFFSET
#define GSSDGT_OFFSET                   KPSSDGT_OFFSET
#define GSSWDT0_OFFSET                  KPSSWDT0_OFFSET
#define GSSWDT1_OFFSET                  KPSSWDT1_OFFSET
#define GSSSTAT_OFFSET                  KPSSSTAT_OFFSET

#define PPSSXOTMR0_OFFSET               0x800
#define PPSSXOTMR1_OFFSET               0x818

#define PPSSTMR0_OFFSET                 0x000
#define PPSSTMR1_OFFSET                 0x018
#define PPSSWDT_OFFSET                  0x800
#define PPSSCLKCTNTL_OFFSET             0x824

#define RPMGPT0_OFFSET                  0x000
#define RPMGPT0_CLK_CTL                 0x010
#define RPMGPT1_OFFSET                  0x040
#define RPMWDT_OFFSET                   0x060
#define RPMTMRSTS_OFFSET                0x100

/* Clock control Bitmaps */
#define PPSSCLKCTNTL_WDGON              (1U << 1)
#define PPSSCLKCTNTL_SLPON              (1U << 1)

#define TMRSTS(x, i)                   ((x) << (8 * i))
#define TMRSTS_WR_PEND(i)              TMRSTS(1U << 3, i)
#define TMRSTS_CLR_PEND(i)             TMRSTS(1U << 2, i)
#define TMRSTS_CLR_ON_MTCH_EN(i)       TMRSTS(1U << 1, i)
#define TMRSTS_EN(i)                   TMRSTS(1U << 0, i)
#define TMRSTS_PEND(i)                 (TMRSTS_WR_PEND(i) | TMRSTS_CLR_PEND(i))

#define RPMTMRSTS_WDOG_EN               (1U << 17)
#define RPMTMRSTS_WDOG_UNMASKED_INT_EN  (1U << 16)
#define RPMTMRSTS_TMR1_WR_PEND          TMRSTS_WR_PEND(1)
#define RPMTMRSTS_TMR1_CLR_PEND         TMRSTS_CLR_PEND(1)
#define RPMTMRSTS_TMR1_CLK_ON_MTCH      TMRSTS_CLR_ON_MTCH(1)
#define RPMTMRSTS_TMR1_PEND             TMRSTS_PEND(1)
#define RPMTMRSTS_TMR1_EN               TMRSTS_EN(1)
#define RPMTMRSTS_TMR0_WR_PEND          TMRSTS_WR_PEND(0)
#define RPMTMRSTS_TMR0_CLR_PEND         TMRSTS_CLR_PEND(0)
#define RPMTMRSTS_TMR0_CLR_ON_MTCH      TMRSTS_CLR_ON_MTCH(0)
#define RPMTMRSTS_TMR0_EN               TMRSTS_EN(0)
#define RPMTMRSTS_TMR0_PEND             TMRSTS_PEND(0)

#define KPSSGPTSTS_TMR1_WR_PEND          TMRSTS_WR_PEND(1)
#define KPSSGPTSTS_TMR1_CLR_PEND         TMRSTS_CLR_PEND(1)
#define KPSSGPTSTS_TMR1_CLK_ON_MTCH      TMRSTS_CLR_ON_MTCH(1)
#define KPSSGPTSTS_TMR1_EN               TMRSTS_EN(1)
#define KPSSGPTSTS_TMR1_PEND             TMRSTS_PEND(1)
#define KPSSGPTSTS_TMR0_WR_PEND          TMRSTS_WR_PEND(0)
#define KPSSGPTSTS_TMR0_CLR_PEND         TMRSTS_CLR_PEND(0)
#define KPSSGPTSTS_TMR0_CLR_ON_MTCH      TMRSTS_CLR_ON_MTCH(0)
#define KPSSGPTSTS_TMR0_EN               TMRSTS_EN(0)
#define KPSSGPTSTS_TMR0_PEND             TMRSTS_PEND(0)

#define GSSGPTSTS_TMR1_WR_PEND          TMRSTS_WR_PEND(1)
#define GSSGPTSTS_TMR1_CLR_PEND         TMRSTS_CLR_PEND(1)
#define GSSGPTSTS_TMR1_CLK_ON_MTCH      TMRSTS_CLR_ON_MTCH(1)
#define GSSGPTSTS_TMR1_EN               TMRSTS_EN(1)
#define GSSGPTSTS_TMR1_PEND             TMRSTS_PEND(1)
#define GSSGPTSTS_TMR0_WR_PEND          TMRSTS_WR_PEND(0)
#define GSSGPTSTS_TMR0_CLR_PEND         TMRSTS_CLR_PEND(0)
#define GSSGPTSTS_TMR0_CLR_ON_MTCH      TMRSTS_CLR_ON_MTCH(0)
#define GSSGPTSTS_TMR0_EN               TMRSTS_EN(0)
#define GSSGPTSTS_TMR0_PEND             TMRSTS_PEND(0)

#define RPMGPT0_DIV(x)                  ((x) - 1)
#define RPMGPT0_DIV4                    RPMGPT0_DIV(4)
#define RPMGPT0_DIV3                    RPMGPT0_DIV(3)
#define RPMGPT0_DIV2                    RPMGPT0_DIV(2)
#define RPMGPT0_DIV1                    RPMGPT0_DIV(1)

#define TMR_CTRL_ON                     (1U << 5)
#define TMR_CTRL_EN                     (1U << 4)
#define TMR_CTRL_MODE(x)                ((x) << 2)
#define TMR_CTRL_MODE_FREE_RUN          TMR_CTRL_MODE(0x0)
#define TMR_CTRL_MODE_ONE_SHOT          TMR_CTRL_MODE(0x1)
#define TMR_CTRL_MODE_PERIODIC          TMR_CTRL_MODE(0x2)
#define TMR_CTRL_MODE_MASK              TMR_CTRL_MODE(0x3)
#define TMR_CTRL_PRESCALE(x)            ((x) << 0)
#define TMR_CTRL_PRESCALE_DIV32         TMR_CTRL_PRESCALE(0x0)
#define TMR_CTRL_PRESCALE_DIV8          TMR_CTRL_PRESCALE(0x1)
#define TMR_CTRL_PRESCALE_DIV4          TMR_CTRL_PRESCALE(0x2)
#define TMR_CTRL_PRESCALE_DIV2          TMR_CTRL_PRESCALE(0x3)
#define TMR_CTRL_PRESCALE_MASK          TMR_CTRL_PRESCALE(0x3)

#define TMR_STAT_MATCH_UPDATE           (1U << 4)
#define TMR_STAT_COUNT_UPDATE           (1U << 3)
#define TMR_STAT_CONTROL_UPDATE         (1U << 2)
#define TMR_STAT_CLR_INT_UPDATE         (1U << 1)
#define TMR_STAT_CLK_CNT_UPDATE         (1U << 0)

/** General bitmaps **/
#define DGTTMR_EN_CLR_ON_MTCH_EN        (1U << 1)
#define DGTTMR_EN_EN                    (1U << 0)
#define DGTTMR_CLK_CTRL(x)              ((x) << 0)
#define DGTTMR_CLK_CTRL_DIV1            DGTTMR_CLK_CTRL(0x0)
#define DGTTMR_CLK_CTRL_DIV2            DGTTMR_CLK_CTRL(0x1)
#define DGTTMR_CLK_CTRL_DIV3            DGTTMR_CLK_CTRL(0x2)
#define DGTTMR_CLK_CTRL_DIV4            DGTTMR_CLK_CTRL(0x3)
#define DGTTMR_CLK_CTRL_MASK            DGTTMR_CLK_CTRL(0x3)

#define WDTRESET                        (1U << 0)
#define WDTFREEZE                       (1U << 0)
#define WDTINTEN_EN                     (1U << 1)
#define WDTINTEN_UNMASKED_INTEN         (1U << 0)
#define WDTSTAT_COUNT(x)                ((x) << 3)
#define WDTSTAT_GETCOUNT(x)             ((x) >> 3)
#define WDTSTAT_CNT_RESET_STAT          (1U << 2)
#define WDTSTAT_FROZEN(x)               (1U << 1)
#define WDTSTAT_EXPIRED(x)              (1U << 0)
#define WDTEXP_SYNC                     (1U << 14)
#define WDTEXP_DATA(x)                  (((x) & 0x3fff) << 0)
#define WDTEXP_GETDATA(x)               (((x) >> 0) & 0x3fff)
#define WDTBARK_SYNC                    (1U << 14)
#define WDTBARK_DATA(x)                 (((x) & 0x3fff) << 0)
#define WDTBARK_GETDATA(x)              (((x) >> 0) & 0x3fff)
#define WDTTESTLOADSTAT_SYNC            (1U << 0)
#define WDTTESTLOAD_LOAD                (1U << 0)
#define WDTTEST_SYNC                    (1U << 14)
#define WDTTEST_LOAD(x)                 (((x) & 0x3fff) << 0)
#define WDTTEST_GETLOAD(x)              (((x) >> 0) & 0x3fff)

#define GPTEN_CLR_ON_MTCH_EN            (1U << 1)
#define GPTEN_EN                        (1U << 0)

/** Helpers **/
#define TIMER_VADDR_OFFSET(base, offset) ((void*)((uintptr_t)base + offset))
#define TIMER_REG(base, offset)          *(volatile uint32_t *)TIMER_VADDR_OFFSET(base, offset)

/** Timer register maps **/
struct tmr_regs {
    uint32_t match;            /* +0x00 */
    uint32_t count;            /* +0x04 */
    uint32_t control;          /* +0x08 */
    uint32_t clear_int;        /* +0x0C */
    uint32_t clear_cnt;        /* +0x10 */
    uint32_t status;           /* +0x14 */
};
typedef volatile struct tmr_regs tmr_regs_t;

struct dgt_regs {
    uint32_t mtch;             /* +0x00 */
    uint32_t cnt;              /* +0x04 */
    uint32_t en;               /* +0x08 */
    uint32_t clr;              /* +0x0C */
    uint32_t clk_ctl;          /* +0x10 */
};
typedef volatile struct dgt_regs dgt_regs_t;

struct wdt_regs {
    uint32_t reset;            /* +0x00 */
    uint32_t freeze;           /* +0x04 */
    uint32_t unmasked_int_en;  /* +0x08 */
    uint32_t status;           /* +0x0C */
    uint32_t expired_width;    /* +0x10 */
    uint32_t bark_time;        /* +0x14 */
    uint32_t test_load_status; /* +0x18 */
    uint32_t test_load;        /* +0x1C */
    uint32_t test;             /* +0x20 */
    /* Only for KRAIT and GSS */
    uint32_t bite_time;        /* +0x24 */
};
typedef volatile struct wdt_regs wdt_regs_t;

struct gpt_regs {
    uint32_t mtch;             /* +0x00 */
    uint32_t cnt;              /* +0x04 */
    uint32_t en;               /* +0x08 */
    uint32_t clr;              /* +0x0C */
};
typedef volatile struct gpt_regs gpt_regs_t;

/**** GPT timer functions ****/
int gpt_timer_start(timer_t *timer)
{
    gpt_regs_t *regs = (gpt_regs_t *)timer->data;
    regs->en |= GPTEN_EN;
    return 0;
}

int gpt_timer_stop(timer_t *timer)
{
    gpt_regs_t *regs = (gpt_regs_t *)timer->data;
    regs->en &= ~GPTEN_EN;
    return 0;
}

uint64_t gpt_get_time(timer_t *timer)
{
    gpt_regs_t *regs = (gpt_regs_t *)timer->data;
    return regs->cnt;
}

int gpt_periodic(timer_t *timer, uint64_t ns)
{
    gpt_regs_t *regs = (gpt_regs_t *)timer->data;
    uintptr_t sts_base = (uintptr_t)timer->data & ~0xfff;
    volatile uint32_t *sts = (volatile uint32_t *)sts_base;
    uint64_t fin;
    switch (timer->id) {
    case TMR_RPM_GPT0 :
        fin = get_xo_hz() / 4;
        break;
    case TMR_RPM_GPT1:
    case TMR_KPSS_GPT0:
    case TMR_KPSS_GPT1:
    case TMR_GSS_GPT0:
    case TMR_GSS_GPT1:
        fin = get_slp_hz();
        break;
    default:
        assert(!"invalid timer for GPT operation");
        return -1;
    }
    /* Clear the timer on match */

    regs->en = GPTEN_CLR_ON_MTCH_EN;
    switch (timer->id) {
    case TMR_RPM_GPT0 :
        while (sts[RPMTMRSTS_OFFSET / 4] & RPMTMRSTS_TMR0_WR_PEND);
        break;
    case TMR_RPM_GPT1:
        while (sts[RPMTMRSTS_OFFSET / 4] & RPMTMRSTS_TMR1_WR_PEND);
        break;
    case TMR_KPSS_GPT0:
    case TMR_GSS_GPT0:
        while (sts[KPSSSTAT_OFFSET / 4] & KPSSGPTSTS_TMR0_WR_PEND);
        break;
    case TMR_KPSS_GPT1:
    case TMR_GSS_GPT1:
        while (sts[KPSSSTAT_OFFSET / 4] & KPSSGPTSTS_TMR1_WR_PEND);
        break;
    default:
        break;
    }
    /* Reset the timer */
    regs->clr = 0xC0FFEE;
    /* Configure match value */
    regs->mtch = (fin * ns) / (1000UL * 1000 * 1000);

    switch (timer->id) {
    case TMR_RPM_GPT0 :
        while (sts[RPMTMRSTS_OFFSET / 4] & RPMTMRSTS_TMR0_PEND);
        break;
    case TMR_RPM_GPT1:
        while (sts[RPMTMRSTS_OFFSET / 4] & RPMTMRSTS_TMR1_PEND);
        break;
    case TMR_KPSS_GPT0:
    case TMR_GSS_GPT0:
        while (sts[KPSSSTAT_OFFSET / 4] & KPSSGPTSTS_TMR0_PEND);
        break;
    case TMR_KPSS_GPT1:
    case TMR_GSS_GPT1:
        while (sts[KPSSSTAT_OFFSET / 4] & KPSSGPTSTS_TMR1_PEND);
        break;
    default:
        break;
    }

    return 0;
}

void gpt_handle_irq(UNUSED timer_t *timer)
{
    /* Nothing to do */
}

/* DGT timer functions */
int dgt_timer_start(timer_t *timer)
{
    dgt_regs_t *regs = (dgt_regs_t *)timer->data;
    regs->en |= DGTTMR_EN_EN;
    return 0;
}

int dgt_timer_stop(timer_t *timer)
{
    dgt_regs_t *regs = (dgt_regs_t *)timer->data;
    regs->en &= ~DGTTMR_EN_EN;
    return 0;
}

uint64_t dgt_get_time(timer_t *timer)
{
    dgt_regs_t *regs = (dgt_regs_t *)timer->data;
    return regs->cnt;
}

int dgt_periodic(timer_t *timer, uint64_t ns)
{
    dgt_regs_t *regs = (dgt_regs_t *)timer->data;
    uint64_t fin_hz = get_tcxo_hz();
    int div;
    /* Disable */
    regs->en = 0;
    /* Configure */
    regs->clr = 0xC0FFEE;
    /* DIV should always be 4 due to clock domain delay */
    regs->clk_ctl = DGTTMR_CLK_CTRL_DIV4;
    div = 4;
    regs->mtch = (fin_hz * ns) / (div * 1000 * 1000 * 1000);
    regs->en = DGTTMR_EN_CLR_ON_MTCH_EN;
    return 0;
}

void dgt_handle_irq(UNUSED timer_t *timer)
{
    /* Nothing to do */
}

/**** TMR ****/
int tmr_timer_start(timer_t *timer)
{
    tmr_regs_t *regs = (tmr_regs_t *)timer->data;
    regs->control |= ~TMR_CTRL_EN;
    return 0;
}

int tmr_timer_stop(timer_t *timer)
{
    tmr_regs_t *regs = (tmr_regs_t *)timer->data;
    regs->control &= ~TMR_CTRL_EN;
    return 0;
}

uint64_t tmr_get_time(timer_t *timer)
{
    tmr_regs_t *regs = (tmr_regs_t *)timer->data;
    return regs->count;
}

int tmr_periodic(timer_t *timer, uint64_t ns)
{
    tmr_regs_t *regs = (tmr_regs_t *)timer->data;
    uint64_t fin_hz;
    /* Find input clock frequency */
    switch (timer->id) {
    case TMR_PPSS_XO_TMR0:
    case TMR_PPSS_XO_TMR1:
        fin_hz = get_tcxo_hz();
        break;
    case TMR_PPSS_SLP_TMR0:
    case TMR_PPSS_SLP_TMR1:
        fin_hz = get_slp_hz();
        break;
    default:
        assert(!"Invalid timer for this call");
        return -1;
    }
    /* Turn on the timer */
    regs->control = TMR_CTRL_ON;
    while (regs->status & TMR_STAT_CONTROL_UPDATE);
    /* Reset and config */
    regs->control |= TMR_CTRL_PRESCALE_DIV4 | TMR_CTRL_MODE_PERIODIC;
    regs->clear_int = 0xC0FFEE;
    regs->clear_cnt = 0xC0FFEE;
    regs->match = (fin_hz * ns) / (4UL * 1000 * 1000 * 1000);
    /* Wait for registers to be updated */
    while (regs->status);
    return 0;
}

void tmr_handle_irq(timer_t *timer)
{
    tmr_regs_t *regs = (tmr_regs_t *)timer->data;
    /* Reset the IRQ */
    regs->clear_int = 0xC0FFEE;
}

/**** WDT ****/
int wdt_timer_start(timer_t *timer)
{
    wdt_regs_t *regs = (wdt_regs_t *)timer->data;
    /* Don't send a reset */
    regs->reset = 0;
    /* Enable the auto-kicker */
    regs->freeze = WDTFREEZE;
    regs->unmasked_int_en = WDTINTEN_EN | WDTINTEN_UNMASKED_INTEN;
    return 0;
}

uint64_t wdt_get_time(timer_t *timer)
{
    wdt_regs_t *regs = (wdt_regs_t *)timer->data;
    return WDTSTAT_GETCOUNT(regs->status);
}

int wdt_periodic(timer_t *timer, uint64_t ns)
{
    wdt_regs_t *regs = (wdt_regs_t *)timer->data;
    uint64_t fin_hz = get_slp_hz();
    uint32_t bark_time;
    /* Disable */
    regs->unmasked_int_en = WDTINTEN_EN;
    /* Enable the auto-kicker */
    regs->freeze = WDTFREEZE;
    /* Don't reset */
    regs->reset = 0;
    /* Set the counter value */
    bark_time = (fin_hz * ns) / (1000 * 1000);
    regs->bark_time = WDTBARK_DATA(bark_time);
    assert(!"Not yet implemented");
    return 0;
}

int timer_init(timer_t *timer, timer_config_t config)
{
    if (config.id < 0 || config.id >= NTIMERS) {
        return EINVAL;
    }

    timer->id = config.id;

    /* Default handlers */
    switch (config.id) {
    case TMR_PPSS_XO_TMR0:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, PPSSXOTMR0_OFFSET);
        break;
    case TMR_PPSS_XO_TMR1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, PPSSXOTMR1_OFFSET);
        break;
    case TMR_PPSS_SLP_TMR0:
        TIMER_REG(config.vaddr, PPSSCLKCTNTL_OFFSET) |= PPSSCLKCTNTL_SLPON;
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, PPSSTMR0_OFFSET);
        break;
    case TMR_PPSS_SLP_TMR1:
        TIMER_REG(config.vaddr, PPSSCLKCTNTL_OFFSET) |= PPSSCLKCTNTL_SLPON;
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, PPSSTMR1_OFFSET);
        break;
    case TMR_PPSS_SLP_WDOG:
        TIMER_REG(config.vaddr, PPSSCLKCTNTL_OFFSET) |= PPSSCLKCTNTL_WDGON;
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, PPSSWDT_OFFSET);
        break;
    /* KPSS */
    case TMR_KPSS_GPT0:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, KPSSGPT0_OFFSET);
        break;
    case TMR_KPSS_GPT1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, KPSSGPT1_OFFSET);
        break;
    case TMR_KPSS_DGT:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, KPSSDGT_OFFSET);
        break;
    case TMR_KPSS_WDT0:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, KPSSGPT0_OFFSET);
        break;
    case TMR_KPSS_WDT1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, KPSSGPT1_OFFSET);
        break;
    /* GSS */
    case TMR_GSS_GPT0:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, GSSGPT0_OFFSET);
        break;
    case TMR_GSS_GPT1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, GSSGPT1_OFFSET);
        break;
    case TMR_GSS_DGT:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, GSSDGT_OFFSET);
        break;
    case TMR_GSS_WDT0:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, GSSGPT0_OFFSET);
        break;
    case TMR_GSS_WDT1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, GSSGPT1_OFFSET);
        break;
    /* RPM */
    case TMR_RPM_GPT0:
        TIMER_REG(config.vaddr, RPMGPT0_CLK_CTL) = RPMGPT0_DIV4;
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, RPMGPT0_OFFSET);
        break;
    case TMR_RPM_GPT1:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, RPMGPT1_OFFSET);
        break;
    case TMR_RPM_WDOG:
        timer->data = TIMER_VADDR_OFFSET(config.vaddr, RPMWDT_OFFSET);
        break;
    default:
        return EINVAL;
    }

    return 0;
}
