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

/* GPT CONTROL REGISTER BITS */
typedef enum {
    /*
     * This bit enables the GPT.
     */
    EN = 0,

    /*
     * By setting this bit, then when EPIT is disabled (EN=0), then
     * both Main Counter and Prescaler Counter freeze their count at
     * current count values.
     */
    ENMOD = 1,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during debug mode
     */
    DBGEN = 2,

    /*
     *  This read/write control bit enables the operation of the GPT
     *  during wait mode
     */
    WAITEN = 3,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during doze mode
     */
    DOZEN = 4,

    /*
     * This read/write control bit enables the operation of the GPT
     *  during stop mode
     */
    STOPEN = 5,

    /*
     * bits 6-8 -  These bits selects the clock source for the
     *  prescaler and subsequently be used to run the GPT counter.
     */
    CLKSRC = 6,

    /*
     * Freerun or Restart mode.
     *
     * 0 Restart mode
     * 1 Freerun mode
     */
    FRR = 9,

    /*
     * Software reset.
     *
     * This bit is set when the module is in reset state and is cleared
     * when the reset procedure is over. Writing a 1 to this bit
     * produces a single wait state write cycle. Setting this bit
     * resets all the registers to their default reset values except
     * for the EN, ENMOD, STOPEN, DOZEN, WAITEN and DBGEN bits in this
     *  control register.
     */
    SWR = 15,

    /* Input capture channel operating modes */
    IM1 = 16, IM2 = 18,

    /* Output compare channel operating modes */
    OM1 = 20, OM2 = 23, OM3 = 26,

    /* Force output compare channel bits */
    FO1 = 29, FO2 = 30, FO3 = 31

} gpt_control_reg;


/* bits in the interrupt/status regiser */
enum gpt_interrupt_register_bits {

    /* Output compare interrupt enable bits */
    OF1IE = 0, OF2IE = 1, OF3IE = 2,

    /* Input capture interrupt enable bits */
    IF1IE = 3, IF2IE = 4,

    /* Rollover interrupt enabled */
    ROV = 5,
};

/* Memory map for GPT. */
struct gpt_map {
    /* gpt control register */
    uint32_t gptcr;
    /* gpt prescaler register */
    uint32_t gptpr;
    /* gpt status register */
    uint32_t gptsr;
    /* gpt interrupt register */
    uint32_t gptir;
    /* gpt output compare register 1 */
    uint32_t gptcr1;
    /* gpt output compare register 2 */
    uint32_t gptcr2;
    /* gpt output compare register 3 */
    uint32_t gptcr3;
    /* gpt input capture register 1 */
    uint32_t gpticr1;
    /* gpt input capture register 2 */
    uint32_t gpticr2;
    /* gpt counter register */
    uint32_t gptcnt;
};

typedef enum {
    PERIODIC,
    ONESHOT,
} gpt_mode_t;

typedef struct gpt {
    volatile struct gpt_map *gpt_map;
    uint64_t counter_start;
    int mode;
    int compare_events;
    uint32_t irq;
    uint32_t prescaler;
} gpt_t;

static int 
gpt_timer_start(const pstimer_t *timer) 
{
    gpt_t *gpt = (gpt_t*) timer->data;


    gpt->gpt_map->gptcr |= BIT(EN);

    return 0;
}


static int
gpt_timer_stop(const pstimer_t *timer) {
    gpt_t *gpt = (gpt_t*) timer->data;
    /* Disable timer. */
    gpt->gpt_map->gptcr = 0;

    return 0;
}

static int
gpt_oneshot_absolute(const pstimer_t *timer UNUSED, uint64_t ns UNUSED)
{
    return ENOSYS;
}


static int
gpt_periodic(const pstimer_t *timer UNUSED, uint64_t ns UNUSED)
{
    return ENOSYS;
}

static int 
gpt_oneshot_relative(const pstimer_t *timer UNUSED, uint64_t ns UNUSED)
{
    return ENOSYS;
}

static void 
gpt_handle_irq(const pstimer_t *timer, uint32_t irq UNUSED) {
    gpt_t *gpt = (gpt_t*) timer->data;

    /* Potentially could be getting interrupts for more reasons
     * driver can't do it though
    */
    if (gpt->gpt_map->gptsr == BIT(ROV)) {
        gpt->compare_events++;
        gpt->gpt_map->gptsr = BIT(ROV);
    }
}

static uint64_t 
gpt_get_time(const pstimer_t *timer) {
    gpt_t *gpt = (gpt_t*) timer->data;
    uint64_t value;
    int extra_event;
    /* read the gpt once */

    value = gpt->gpt_map->gptcnt;

    /* check to see if an interrupt happened */
    extra_event = !!gpt->gpt_map->gptsr;

    /* if an interrupt has happened, read the counter again in case we
     * read the value before the interrupt  */
    if (extra_event) {
        value = gpt->gpt_map->gptcnt;
    }
    uint64_t total = (uint64_t) ((((uint64_t)(extra_event + gpt->compare_events)) << 32) + value);
    uint64_t ns = (total / (uint64_t)IPG_FREQ) * 1000llu;
    return ns;
}

static uint32_t
gpt_get_nth_irq(const pstimer_t *timer UNUSED, uint32_t n UNUSED) 
{
    return GPT1_INTERRUPT;
}

static pstimer_t singleton_timer;
static gpt_t singleton_gpt;

pstimer_t *
gpt_get_timer(gpt_config_t *config)
{
    pstimer_t *timer = &singleton_timer;
    gpt_t *gpt = &singleton_gpt;

    timer->properties.upcounter = true;
    /* More can probably be done with this timer
     * but this driver can only count up
     */
    timer->properties.timeouts = false;
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
    gpt->compare_events = 0;

    /* Disable GPT. */
    gpt->gpt_map->gptcr = 0;

    /* Configure GPT. */
    gpt->gpt_map->gptcr |= BIT(ENMOD); /* Reset to 0 on disable */
    gpt->gpt_map->gptcr = 0 | BIT(SWR); /* Reset the GPT */
    gpt->gpt_map->gptcr = BIT(FRR) | BIT(CLKSRC); /* GPT can do more but for this just set it as free running
    so we can tell the time */
    gpt->gpt_map->gptir = BIT(ROV); /* Interrupt when the timer overflows */
    gpt->gpt_map->gptpr = config->prescaler; /* Set the prescaler */

    return timer;
}
