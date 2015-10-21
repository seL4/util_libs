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
#include <platsupport/mach/gpt.h>

#define TIMER_INTERVAL_TICKS(ns) ((uint32_t)(1ULL * (ns) * CLK_FREQ / 1000 / 1000 / 1000))

typedef enum {

    /* Start or stop the timer */
    ST = 0,

    /* Autoreload mode */
    AR = 1,

    /* Prescale value
     * Timer is prescaled 2^(PTV+1).
     * EG: PTV = 3. Timer increases every 16 clock periods.
     */
    PTV = 2,

    /* Enable prescaler */
    PRE = 5,

    /* Compare enabled */
    CE = 6,

    /* Pulse-width-modulation output pin default setting when
     * counter is stopped or trigger output mode is set to no trigger
     *  0x0: Default value of PWM_out output: 0
     *  0x1: Default value of PWM_out output: 1
     */
    SCPWM = 7,

    /* Transition capture mode
     * 0x0: No capture
     * 0x1: Capture on rising edges of EVENT_CAPTURE pin.
     * 0x2: Capture on falling edges of EVENT_CAPTURE pin.
     * 0x3: Capture on both edges of EVENT_CAPTURE pin.
     */
    TCM = 8,

    /* Trigger output mode
     * 0x0: No trigger
     * 0x1: Overflow trigger
     * 0x2: Overflow and match trigger
     * 0x3: Reserved
     */
    TRG  = 10,

    /* Pulse or toggle select bit. Pulse 0. Toggle 1. */
    PT = 12,


    /* Capture mode select bit (first/second)
     * 0x0: Capture the first enabled capture event in TCAR1.
     * 0x1: Capture the second enabled capture event in TCAR2.
     */
    CAPT_MODE = 13,

    /* PWM output/event detection input pin direction control:
     * 0x0: Configures the pin as an output (needed when PWM mode is required)
     * 0x1: Configures the pin as an input (needed when capture mode is required)
     */
    GPO_CFG = 14
} gpt_control_reg;

typedef enum {

    /* Enable match interrupt */
    MAT_IT_ENA = 0,

    /* Enable overflow interrupt */
    OVF_IT_ENA = 1,

    /* Enable capture interrupt */
    TCAR_IT_ENA = 2

} gpt_int_en_reg;

typedef enum {
    /* General guide for all three flags:
     * Read 1: Interrupt pending
     * Read 0: No Interrupt pending
     * Write 1: Clear flag
     * Write 0: No change
     */

    /* Enable match interrupt */
    MAT_IT_FLAG = 0,

    /* Enable overflow interrupt */
    OVF_IT_FLAG = 1,

    /* Enable capture interrupt */
    TCAR_IT_FLAG = 2

} gpt_int_stat_reg;

typedef enum {

    /* PWM output/event detection input pin direction control:
     * 0x0: Configures the pin as an output (needed when PWMmode is required)
     * 0x1: Configures the pin as an input (needed when capture mode is required)
     */
    AUTOIDLE = 0,

    /* Software reset. This bit is automatically reset by the hardware.
     * During reads, it always returns 0.
     * 0x0: Normal mode
     * 0x1: The module is reset.
     */
    SOFTRESET = 1,

    /* Software reset. This bit is automatically reset by the RW 0
     * hardware. During reads, it always returns 0.
     * 0x0: Normal mode
     * 0x1: The module is reset.
     */
    ENAWAKEUP = 3

                //TODO Add extra definitions

} gpt_cfg_reg;

/* Memory map for GPT */
struct gpt_map {
    uint32_t tidr;   // GPTIMER_TIDR 0x00
    uint32_t padding1[3];
    uint32_t cfg;    // GPTIMER_CFG 0x10
    uint32_t tistat; // GPTIMER_TISTAT 0x14
    uint32_t tisr;   // GPTIMER_TISR 0x18
    uint32_t tier;   // GPTIMER_TIER 0x1C
    uint32_t twer;   // GPTIMER_TWER 0x20
    uint32_t tclr;   // GPTIMER_TCLR 0x24
    uint32_t tcrr;   // GPTIMER_TCRR 0x28
    uint32_t tldr;   // GPTIMER_TLDR 0x2C
    uint32_t ttgr;   // GPTIMER_TTGR 0x30
    uint32_t twps;   // GPTIMER_TWPS 0x34
    uint32_t tmar;   // GPTIMER_TMAR 0x38
    uint32_t tcar1;  // GPTIMER_TCAR1 0x3C
    uint32_t tsicr;  // GPTIMER_TSICR 0x40
    uint32_t tcar2;  // GPTIMER_TCAR2 0x44
    uint32_t tpir;   // GPTIMER_TPIR 0x48
    uint32_t tnir;   // GPTIMER_TNIR 0x4C
    uint32_t tcvr;   // GPTIMER_TCVR 0x50
    uint32_t tocr;   // GPTIMER_TOCR 0x54
    uint32_t towr;   // GPTIMER_TOWR 0x58
};

typedef enum {
    PERIODIC,
    ONESHOT
} gpt_mode_t;

typedef struct gpt {
    volatile struct gpt_map *gpt_map;
    uint64_t counter_start;
    uint32_t irq;
    gpt_id_t id;
    uint32_t prescaler;
} gpt_t;

static int
gpt_timer_start(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;


    gpt->gpt_map->tclr |= BIT(ST);

    return 0;
}


static int
gpt_timer_stop(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;
    /* Disable timer. */
    gpt->gpt_map->tclr = 0;

    return 0;
}

void configure_timeout(const pstimer_t *timer, uint64_t ns)
{

    gpt_t *gpt = (gpt_t*) timer->data;

    /* Stop time. */
    gpt->gpt_map->tclr = 0;

    /* Reset timer. */
    gpt->gpt_map->cfg = BIT(SOFTRESET);

    while (!gpt->gpt_map->tistat); /* Wait for GPT to reset */

    gpt->gpt_map->tclr = (gpt->prescaler << PTV); /* Set the prescaler */
    gpt->gpt_map->tclr = BIT(PRE); /* Enable the prescaler */

    /* Enable interrupt on overflow. */
    gpt->gpt_map->tier = BIT(OVF_IT_ENA);

    /* Set the reload value. */
    gpt->gpt_map->tldr = ~0UL - TIMER_INTERVAL_TICKS(ns);

    /* Reset the read register. */
    gpt->gpt_map->tcrr = ~0UL - TIMER_INTERVAL_TICKS(ns);

    /* Clear pending overflows. */
    gpt->gpt_map->tisr = BIT(OVF_IT_FLAG);
}

static int
gpt_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not implemented");
    return ENOSYS;
}


static int
gpt_periodic(const pstimer_t *timer, uint64_t ns)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    configure_timeout(timer, ns);

    /* Set autoreload and start the timer. */
    gpt->gpt_map->tclr = BIT(AR) | BIT(ST);

    return 0;
}

static int
gpt_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    configure_timeout(timer, ns);

    /* Set start the timer with no autoreload. */
    gpt->gpt_map->tclr = BIT(ST);

    return 0;
}

static void
gpt_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    /* Potentially could be getting interrupts for more reasons
     * driver can't do it though
    */
    if (gpt->gpt_map->tisr == BIT(OVF_IT_FLAG)) {
        gpt->gpt_map->tisr = BIT(OVF_IT_FLAG);
    }
}

static uint64_t
gpt_get_time(const pstimer_t *timer)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    uint32_t value = gpt->gpt_map->tcrr;

    uint64_t ns = ((uint64_t) value / (uint64_t)CLK_FREQ) * NS_IN_US * (gpt->prescaler + 1);
    return ns;
}

static uint32_t
gpt_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    gpt_t *gpt = (gpt_t*) timer->data;

    return GPT1_INTERRUPT + (gpt->id - GPT1);
}

static pstimer_t singleton_timer;
static gpt_t singleton_gpt;

pstimer_t *
gpt_get_timer(gpt_config_t *config)
{
    pstimer_t *timer = &singleton_timer;
    gpt_t *gpt = &singleton_gpt;

    if (config->prescaler > 7) {
        fprintf(stderr, "Prescaler value set too large for device, value: %d, max 7", config->prescaler);
        return NULL;
    }

    timer->properties.upcounter = true;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;

    timer->data = (void *) gpt;
    timer->start = gpt_timer_start;
    timer->stop = gpt_timer_stop;
    timer->get_time = gpt_get_time;
    timer->oneshot_absolute = gpt_oneshot_absolute;
    timer->oneshot_relative = gpt_oneshot_relative;
    timer->periodic = gpt_periodic;
    timer->handle_irq = gpt_handle_irq;
    timer->get_nth_irq = gpt_get_nth_irq;

    gpt->gpt_map = (volatile struct gpt_map*)config->vaddr;

    gpt->prescaler = config->prescaler;

    /* Disable GPT. */
    gpt->gpt_map->tclr = 0;

    /* Configure GPT. */

    /* Perform a soft reset */
    gpt->gpt_map->cfg = BIT(SOFTRESET);

    while (!gpt->gpt_map->tistat); /* Wait for timer to reset */

    return timer;
}
