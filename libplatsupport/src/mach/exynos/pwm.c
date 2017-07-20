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
#include <platsupport/mach/pwm.h>

#define ACLK_66 66 /* MHz */

#define T4_ENABLE          BIT(20)
#define T4_MANUALRELOAD    BIT(21)
#define T4_AUTORELOAD      BIT(22)

#define T0_ENABLE          BIT(0)
#define T0_MANUALRELOAD    BIT(1)
#define T0_AUTORELOAD      BIT(3)

/* TCFG0 */
#define T234_PRESCALE(x)   ((x) << 8)
#define T234_PRESCALE_MASK T234_PRESCALE(0xff)
#define T01_PRESCALE(x)   (x)
#define T01_PRESCALE_MASK T01_PRESCALE(0xff)

/* TCFG1 */
#define T4_DIVISOR(x)      ((x) << 16)
#define T4_DIVISOR_MASK    T4_DIVISOR(0xf)
#define T0_DIVISOR(x)      (x)
#define T0_DIVISOR_MASK    T0_DIVISOR(0xf)

/* TINT_CSTAT */
#define INT_ENABLE(x)      BIT(x)
#define INT_STAT(x)        BIT((x) + 5)
#define INT_ENABLE_ALL     ( INT_ENABLE(0) | INT_ENABLE(1) | INT_ENABLE(2) \
                           | INT_ENABLE(3) | INT_ENABLE(4)                 )

void configure_timeout(pwm_t *pwm, uint64_t ns, int timer_number, bool periodic)
{
    assert((timer_number == 0) | (timer_number == 4)); // Only these timers are currently supported
    uint32_t v;

    /* Disable timer. */
    if (timer_number == 4) {
        pwm->pwm_map->tcon &= ~(T4_ENABLE);
    } else {
        pwm->pwm_map->tcon &= ~(T0_ENABLE);
    }

    /* Enable interrupt on overflow. */
    if (timer_number == 4) {
        pwm->pwm_map->tint_cstat |= INT_ENABLE(4);
    } else {
        pwm->pwm_map->tint_cstat |= INT_ENABLE(0);
    }

    /* clear the scale */
    if (timer_number == 4) {
        pwm->pwm_map->tcfg0 &= ~(T234_PRESCALE_MASK);
        pwm->pwm_map->tcfg1 &= ~(T4_DIVISOR_MASK);
    } else {
        pwm->pwm_map->tcfg0 &= ~(T01_PRESCALE_MASK);
        pwm->pwm_map->tcfg1 &= ~(T0_DIVISOR_MASK);
    }

    /* Calculate the scale and reload values. */
    uint32_t div = 0; /* Not implemented */
    uint32_t prescale = ns * ACLK_66 / 1000 / 100000000UL;
    uint32_t cnt = ns * ACLK_66 / 1000 / (prescale + 1) / (BIT(div));
    assert(prescale <= 0xff); /* if this fails, we need to implement div */
    assert(div <= 0xf);

    /* set scale and reload values */
    if (timer_number == 4) {
        pwm->pwm_map->tcfg0 |= T234_PRESCALE(prescale);
        pwm->pwm_map->tcfg1 &= T4_DIVISOR(div);
        pwm->pwm_map->tcntB4 = cnt;
    } else {
        pwm->pwm_map->tcfg0 |= T01_PRESCALE(prescale);
        pwm->pwm_map->tcfg1 &= T0_DIVISOR(div);
        pwm->pwm_map->tcntB0 = cnt;
    }

    /* load tcntB4 by flushing the double buffer */
    if (timer_number == 4) {
        pwm->pwm_map->tcon |= T4_MANUALRELOAD;
        pwm->pwm_map->tcon &= ~(T4_MANUALRELOAD);
    } else {
        pwm->pwm_map->tcon |= T0_MANUALRELOAD;
        pwm->pwm_map->tcon &= ~(T0_MANUALRELOAD);
    }

    /* Clear pending overflows. */
    v = pwm->pwm_map->tint_cstat;
    if (timer_number == 4) {
        v = (v & INT_ENABLE_ALL) | INT_STAT(4);
    } else {
        v = (v & INT_ENABLE_ALL) | INT_STAT(0);
    }
    pwm->pwm_map->tint_cstat = v;

    if (periodic) {
        if (timer_number == 4) {
            pwm->pwm_map->tcon |= T4_AUTORELOAD;
        } else {
            pwm->pwm_map->tcon |= T0_AUTORELOAD;
        }
    }
}

int pwm_start(pwm_t *pwm)
{
    /* start the timer */
    pwm->pwm_map->tcon |= T4_ENABLE;

    /* Start timer0 for get_time */
    configure_timeout(pwm, NS_IN_S, 0, true);
    /* Set autoreload and start the timer. */
    pwm->pwm_map->tcon |= T0_ENABLE;

    return 0;
}

int pwm_stop(pwm_t *pwm)
{
    /* Disable timer. */
    pwm->pwm_map->tcon &= ~(T4_ENABLE | T0_ENABLE);

    /* load tcntB4 and tcntB0 by flushing the double buffer */
    pwm->pwm_map->tcon |= T4_MANUALRELOAD;
    pwm->pwm_map->tcon &= ~(T4_MANUALRELOAD);
    pwm->pwm_map->tcon |= T0_MANUALRELOAD;
    pwm->pwm_map->tcon &= ~(T0_MANUALRELOAD);

    /* disable interrupts */
    pwm->pwm_map->tint_cstat &= ~(INT_ENABLE(4) | INT_ENABLE(0));

    /* ack interrupt */
    pwm->pwm_map->tint_cstat |= ~(INT_STAT(4) | INT_STAT(0));

    return 0;
}

int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic)
{
    configure_timeout(pwm, ns, 4, periodic);

    /* start the timer. */
    pwm->pwm_map->tcon |= T4_ENABLE;

    return 0;
}

void pwm_handle_irq(pwm_t *pwm, uint32_t irq)
{
    uint32_t v;
    v = pwm->pwm_map->tint_cstat;
    if (irq == PWM_T4_INTERRUPT) {
        if (v & INT_STAT(0)) {
            pwm->time_h++;
            v = (v & INT_ENABLE_ALL) | INT_STAT(0);
        }
    } else if (irq == PWM_T0_INTERRUPT) {
        if (v & INT_STAT(4)) {
            v = (v & INT_ENABLE_ALL) | INT_STAT(4);
        }
    }
    pwm->pwm_map->tint_cstat = v;
}

uint64_t pwm_get_time(pwm_t *pwm)
{
    pwm_handle_irq(pwm, 0); // Ensure the time is up to date
    uint64_t time_l = (pwm->pwm_map->tcntO0 / (ACLK_66 / 1000.0)); // Clk is in MHz
    return pwm->time_h * NS_IN_S + (NS_IN_S - time_l);
}

int pwm_init(pwm_t *pwm, pwm_config_t config)
{
    pwm->pwm_map = (volatile struct pwm_map*) config.vaddr;
    return 0;
}
