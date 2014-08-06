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
#include <platsupport/mach/epit.h>
#include <platsupport/plat/timer.h>

/* EPIT CONTROL REGISTER BITS */
typedef enum {
    /*
     * This bit enables the EPIT.
     */
    EN = 0,

    /*
     * By setting this bit, then when EPIT is disabled (EN=0), then
     * both Main Counter and Prescaler Counter freeze their count at
     * current count values.
     */
    ENMOD = 1,

    /*
     *  This bit enables the generation of interrupt when a compare
     *  event occurs
     */
    OCIEN = 2,

    /*
     * This bit is cleared by hardware reset. It controls whether the
     * counter runs in free running mode OR set and forget mode.
     */
    RLD = 3,

    /* 
     * Bits 4 - 15 determine the prescaler value by which the clock is divided
     * before it goes into the counter. 
     * 
     * The prescaler used is the value in these bits + 1. ie:
     * 
     * 0x00 divide by 1
     * 0x01 divide by 2
     * 0x10 divide by 3
     *  .
     *  .
     *  .
     * 0xFFF divide by 4096
     * 
     */
    PRESCALER = 4,

    /*
     * This bit controls the counter data when the modulus register is
     * written. When this bit is set, all writes to the load register
     * will overwrite the counter contents and the counter will
     * subsequently start counting down from the programmed value.
     */
    IOVW = 17,

    /*
     * These bits select the clock input used to run the counter. After
     * reset, the system functional clock is selected. The input clock
     * can also be turned off if these bits are set to 00. This field
     * value should only be changed when the EPIT is disabled.
     */
    CLKSRC = 24
} epit_control_reg;


enum IPGConstants {
    IPG_CLK = 1, IPG_CLK_HIGHFREQ = 2, IPG_CLK_32K = 3
};

/* Memory map for EPIT (Enhanced Periodic Interrupt Timer). */
struct epit_map {
    /* epit control register */
    uint32_t epitcr;
    /* epit status register */
    uint32_t epitsr;
    /* epit load register */
    uint32_t epitlr;
    /* epit compare register */
    uint32_t epitcmpr;
    /* epit counter register */
    uint32_t epitcnt;
};

typedef enum {
    PERIODIC,
    ONESHOT
} epit_mode_t;

typedef struct epit {
    volatile struct epit_map *epit_map;
    uint64_t counter_start;
    int mode;
    uint32_t irq;
    uint32_t prescaler;
} epit_t;

static int 
epit_timer_start(const pstimer_t *timer) 
{
    epit_t *epit = (epit_t*) timer->data;

    /* Enable EPIT. */
    epit->epit_map->epitcr |= 1;

    return 0;
}


static int 
epit_timer_stop(const pstimer_t *timer) {
    epit_t *epit = (epit_t*) timer->data;
    /* Disable timer irq. */
    epit->epit_map->epitcr &= ~(BIT(EN));

    return 0;
}

static int
configure_epit(const pstimer_t *timer, uint64_t ns) {
    epit_t *epit = (epit_t*) timer->data;

     /* Set counter modulus - this effectively sets the timeouts to us but doesn't 
      * overflow as fast. */
    uint64_t counterValue =  (uint64_t) (IPG_FREQ / (epit->prescaler + 1)) * (ns / 1000ULL);
    if (counterValue >= (1ULL << 32)) {
        /* Counter too large to be stored in 32 bits. */
        return EINVAL;
    }

    epit->counter_start = ns;

    /* hardware has a race condition where the epitlr won't be set properly
     * - keep trying until it works
     */
    epit->epit_map->epitlr = counterValue;
    while (epit->epit_map->epitlr != counterValue) {
        epit->epit_map->epitlr = counterValue;
    }

     /* turn it on (just in case it was off) */
     epit->epit_map->epitcr |= BIT(EN);
     return 0;
}

static int
epit_oneshot_absolute(const pstimer_t *timer UNUSED, uint64_t ns UNUSED)
{
    /* epit is a cound-down timer, can't set relative timeouts */
    return ENOSYS;
}


static int
epit_periodic(const pstimer_t *timer, uint64_t ns) {
    epit_t *epit = (epit_t*) timer->data;

    epit->mode = PERIODIC;
    return configure_epit(timer, ns);
}

static int 
epit_oneshot_relative(const pstimer_t *timer, uint64_t ns) {
    epit_t *epit = (epit_t*) timer->data;

    epit->mode = ONESHOT;
    return configure_epit(timer,ns);
}

static void 
epit_handle_irq(const pstimer_t *timer, uint32_t irq) {
    epit_t *epit = (epit_t*) timer->data;

    assert(irq == epit->irq);

    if (epit->epit_map->epitsr) {
        epit->epit_map->epitsr = 1;

        if(epit->mode != PERIODIC) {
            /* disable the epit if we don't want it to be periodic */
            /* this has to be done as the epit is configured to 
             * reload the timer value after irq - this isn't desired
             * if we are periodic */
            epit->epit_map->epitcr &= ~(BIT(EN));
        }
    }
}

static uint64_t 
epit_get_time(const pstimer_t *timer) {
    epit_t *epit = (epit_t*) timer->data;
    uint64_t value;

    /* read the epit */
    value = epit->epit_map->epitcnt;
    uint64_t ns = (value / (uint64_t)IPG_FREQ) * 1000llu;

    return epit->counter_start - ns;
}

static uint32_t
epit_get_nth_irq(const pstimer_t *timer, uint32_t n) 
{
    epit_t *epit = (epit_t*) timer->data;

    if (n == 0) {
        return epit->irq;
    }

    return 0;
}


static pstimer_t singleton_timer;
static epit_t singleton_epit;

pstimer_t *
epit_get_timer(epit_config_t *config)
{

    /* check the irq */
    if (config->irq != EPIT1_INTERRUPT && config->irq != EPIT2_INTERRUPT) {
        fprintf(stderr, "Invalid irq %u for epit, expected %u or %u\n", config->irq, 
                EPIT1_INTERRUPT, EPIT2_INTERRUPT);
        return NULL;
    }

    pstimer_t *timer = &singleton_timer;
    epit_t *epit = &singleton_epit;

    timer->properties.upcounter = false;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;

    timer->data = (void *) epit;
    timer->start = epit_timer_start;
    timer->stop = epit_timer_stop;
    timer->get_time = epit_get_time;
    timer->oneshot_absolute = epit_oneshot_absolute;
    timer->oneshot_relative = epit_oneshot_relative;
    timer->periodic = epit_periodic;
    timer->handle_irq = epit_handle_irq;
    timer->get_nth_irq = epit_get_nth_irq;

    epit->irq = config->irq;
    epit->epit_map = (volatile struct epit_map*)config->vaddr;
    epit->prescaler = config->prescaler;

    /* Disable EPIT. */
    epit->epit_map->epitcr = 0;

    /* Configure EPIT. */
    epit->epit_map->epitcr = (IPG_CLK << CLKSRC) | /* Clock source = IPG */
    (config->prescaler << PRESCALER) | /* Set the prescaler */
    BIT(IOVW) | /* Overwrite counter immediately on write */
    BIT(RLD) | /* Reload counter from modulus register on overflow */
    BIT(OCIEN) | /* Enable interrupt on comparison event */
    BIT(ENMOD) | /* Count from modulus on restart */
    0;

    /* Interrupt when compare with 0. */
    epit->epit_map->epitcmpr = 0;

    return timer;
}
