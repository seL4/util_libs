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
#include <platsupport/mach/pwm.h>
#include <platsupport/plat/timer.h>

#define ACLK_100 100 /* MHz */

#define T4_ENABLE          BIT(20)
#define T4_MANUALRELOAD    BIT(21)
#define T4_AUTORELOAD      BIT(22)

     /* TCFG0 */
#define T234_PRESCALE(x)   ((x) << 8)
#define T234_PRESCALE_MASK T234_PRESCALE(0xff)

     /* TCFG1 */
#define T4_DIVISOR(x)      ((x) << 16)
#define T4_DIVISOR_MASK    T4_DIVISOR(0xf)

     /* TINT_CSTAT */
#define INT_ENABLE(x)      BIT(x)
#define INT_STAT(x)        BIT((x) + 5)
#define INT_ENABLE_ALL     ( INT_ENABLE(0) | INT_ENABLE(1) | INT_ENABLE(2) \
                           | INT_ENABLE(3) | INT_ENABLE(4)                 )


/* Memory map for pwm */
struct pwm_map {
    uint32_t tcfg0;
    uint32_t tcfg1;
    uint32_t tcon;
    uint32_t tcntB0;
    uint32_t tcmpB0;
    uint32_t tcntO0;
    uint32_t tcntB1;
    uint32_t tcmpB1;
    uint32_t tcntO1;
    uint32_t tcntB2;
    uint32_t tcmpB2;
    uint32_t tcntO2;
    uint32_t tcntB3;
    uint32_t tcmpB3;
    uint32_t tcntO3;
    uint32_t tcntB4;
    uint32_t tcntO4;
    uint32_t tint_cstat;
};

typedef struct pwm {
    volatile struct pwm_map *pwm_map;
} pwm_t;

static int 
pwm_timer_start(const pstimer_t *timer) 
{
    pwm_t *pwm = (pwm_t *) timer->data;
 
    /* start the timer */
    pwm->pwm_map->tcon |= T4_ENABLE;

    return 0;
}


static int
pwm_timer_stop(const pstimer_t *timer) {
    pwm_t *pwm = (pwm_t*) timer->data;

    /* Disable timer. */
    pwm->pwm_map->tcon &= ~(T4_ENABLE);

    /* load tcntB4 by flushing the double buffer */
    pwm->pwm_map->tcon |= T4_MANUALRELOAD;
    pwm->pwm_map->tcon &= ~(T4_MANUALRELOAD);

    /* disable interrupts */
    pwm->pwm_map->tint_cstat &= ~(INT_ENABLE(4));
            
    /* ack interrupt */
    pwm->pwm_map->tint_cstat |= ~(INT_STAT(4));

    return 0;
}

void configure_timeout(const pstimer_t *timer, uint64_t ns) {
    uint32_t v;

    pwm_t *pwm = (pwm_t*) timer->data;

    /* Disable timer. */
    pwm->pwm_map->tcon &= ~(T4_ENABLE);

    /* Enable interrupt on overflow. */
    pwm->pwm_map->tint_cstat |= INT_ENABLE(4);

    /* clear the scale */
    pwm->pwm_map->tcfg0 &= ~(T234_PRESCALE_MASK);
    pwm->pwm_map->tcfg1 &= ~(T4_DIVISOR_MASK);
                        
    /* Calculate the scale and reload values. */
    uint32_t div = 0; /* Not implemented */
    uint32_t prescale = ns * ACLK_100 / 1000 / 100000000UL;
    uint32_t cnt = ns * ACLK_100 / 1000 / (prescale + 1) / (1 << div);
    assert(prescale <= 0xff); /* if this fails, we need to implement div */
    assert(div <= 0xf);
                                            
    /* set scale and reload values */
    pwm->pwm_map->tcfg0 |= T234_PRESCALE(prescale);
    pwm->pwm_map->tcfg1 &= T4_DIVISOR(div);
    pwm->pwm_map->tcntB4 = cnt;
                                             
    /* load tcntB4 by flushing the double buffer */
    pwm->pwm_map->tcon |= T4_MANUALRELOAD;
    pwm->pwm_map->tcon &= ~(T4_MANUALRELOAD);
                                                              
    /* Clear pending overflows. */
    v = pwm->pwm_map->tint_cstat;
    v = (v & INT_ENABLE_ALL) | INT_STAT(4);
    pwm->pwm_map->tint_cstat = v;
                                                                    

}

static int
pwm_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not supported");
    return ENOSYS;
}


static int
pwm_periodic(const pstimer_t *timer, uint64_t ns)
{
    pwm_t *pwm = (pwm_t*) timer->data;

    configure_timeout(timer, ns);

    /* Set autoreload and start the timer. */
    pwm->pwm_map->tcon = T4_AUTORELOAD | T4_ENABLE;

    return 0;
}

static int 
pwm_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    pwm_t *pwm = (pwm_t*) timer->data;

    configure_timeout(timer, ns);

    /* Start the timer. */
    pwm->pwm_map->tcon = T4_ENABLE;

    return 0;
}

static void 
pwm_handle_irq(const pstimer_t *timer, uint32_t irq) {
    pwm_t *pwm = (pwm_t*) timer->data;
    uint32_t v;
    v = pwm->pwm_map->tint_cstat;
    v = (v & INT_ENABLE_ALL) | INT_STAT(4);
    pwm->pwm_map->tint_cstat = v;
}


static uint64_t 
pwm_get_time(const pstimer_t *timer) {
    assert(!"Not yet implemented");
    return 0;
}

static uint32_t
pwm_get_nth_irq(const pstimer_t *timer, uint32_t n) 
{
    return PWM_T4_INTERRUPT;
}

static pstimer_t singleton_timer;
static pwm_t singleton_pwm;

pstimer_t *
pwm_get_timer(pwm_config_t *config)
{
    pstimer_t *timer = &singleton_timer;
    pwm_t *pwm = &singleton_pwm;

    timer->properties.upcounter = false;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;

    timer->data = (void *) pwm;
    timer->start = pwm_timer_start;
    timer->stop = pwm_timer_stop;
    timer->get_time = pwm_get_time;
    timer->oneshot_absolute = pwm_oneshot_absolute;
    timer->oneshot_relative = pwm_oneshot_relative;
    timer->periodic = pwm_periodic;
    timer->handle_irq = pwm_handle_irq;
    timer->get_nth_irq = pwm_get_nth_irq;

    pwm->pwm_map = (volatile struct pwm_map*) config->vaddr;

    return timer;
}
