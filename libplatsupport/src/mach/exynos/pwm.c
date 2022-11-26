/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/mach/pwm.h>
#include <platsupport/ltimer.h>
#include <platsupport/fdt.h>

#if defined CONFIG_PLAT_EXYNOS5
#define CLK_FREQ 66ull /* MHz */
#else
#define CLK_FREQ 100ull /* MHz */
#endif

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

/* How many interrupts must be defined in device tree */
#define PWM_FDT_IRQ_COUNT       (5u)

static void configure_timeout(pwm_t *pwm, uint64_t ns, int timer_number, bool periodic)
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
    uint64_t ticks = (ns * CLK_FREQ) / 1000;
    uint32_t prescale = ticks >> (32);
    uint32_t cnt = ticks / (prescale + 1) / (BIT(div));

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

/*
 * We will use timer 0 for timekeeping overflows, timer 4 for user-requested timeouts.
 */
static int pwm_start(pwm_t *pwm)
{
    /* start the timer */
    pwm->pwm_map->tcon |= T4_ENABLE;

    /* Start timer0 for get_time */
    configure_timeout(pwm, NS_IN_S, 0, true);
    /* Set autoreload and start the timer. */
    pwm->pwm_map->tcon |= T0_ENABLE;
    pwm->time_h = 0;

    return 0;
}

static int pwm_stop(pwm_t *pwm)
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

/*
 * Check if a timekeeping overflow interrupt is pending, and increment counter if so.
 */
static void pwm_check_timekeeping_overflow(pwm_t *pwm)
{
    uint32_t v = pwm->pwm_map->tint_cstat;
    if (v & INT_STAT(0)) {
        pwm->time_h++;
        v = (v & INT_ENABLE_ALL) | INT_STAT(0);
    }
    pwm->pwm_map->tint_cstat = v;
}

static void pwm_handle_irq0(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    pwm_t *pwm = data;
    pwm_check_timekeeping_overflow(data);
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (pwm->user_cb_fn) {
        pwm->user_cb_fn(pwm->user_cb_token, LTIMER_OVERFLOW_EVENT);
    }
}

static void pwm_handle_irq4(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    pwm_t *pwm = data;
    uint32_t v = pwm->pwm_map->tint_cstat;
    if (v & INT_STAT(4)) {
        v = (v & INT_ENABLE_ALL) | INT_STAT(4);
    }
    pwm->pwm_map->tint_cstat = v;
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (pwm->user_cb_fn) {
        pwm->user_cb_fn(pwm->user_cb_token, LTIMER_TIMEOUT_EVENT);
    }
}

uint64_t pwm_get_time(pwm_t *pwm)
{
    uint64_t hi = pwm->time_h;
    uint64_t time_l = ((pwm->pwm_map->tcntO0 / CLK_FREQ) * 1000.0); // Clk is in MHz
    pwm_check_timekeeping_overflow(pwm); // Ensure the time is up to date
    if (hi != pwm->time_h) {
        time_l = ((pwm->pwm_map->tcntO0 / CLK_FREQ) * 1000.0);
    }
    return pwm->time_h * NS_IN_S + (NS_IN_S - time_l);
}

static int pwm_walk_registers(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    pwm_t *pwm = token;
    void *mmio_vaddr;

    /* assert only one entry in device tree node */
    if (curr_num != 0 || num_regs != 1) {
        ZF_LOGE("Too many registers in timer device tree node");
        return -ENODEV;
    }

    mmio_vaddr = ps_pmem_map(&pwm->ops, pmem, false, PS_MEM_NORMAL);
    if (mmio_vaddr == NULL) {
        ZF_LOGE("Unable to map timer device");
        return -ENODEV;
    }

    pwm->pwm_map = mmio_vaddr;
    pwm->pmem = pmem;

    return 0;
}

static int pwm_walk_irqs(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    pwm_t *pwm = token;
    irq_id_t irq_id;

    /* the device has 5 timers */
    if (num_irqs != PWM_FDT_IRQ_COUNT) {
        ZF_LOGE("Expected interrupts count of 5, have %zu", num_irqs);
        return -ENODEV;
    }

    /* we only support 0 and 4, so ignore others */
    switch (curr_num) {
    case 0:
        irq_id = ps_irq_register(&pwm->ops.irq_ops, irq, pwm_handle_irq0, pwm);
        if (irq_id < 0) {
            ZF_LOGE("Unable to register timer irq0");
            return irq_id;
        }
        pwm->t0_irq = irq_id;
        break;

    case 4:
        irq_id = ps_irq_register(&pwm->ops.irq_ops, irq, pwm_handle_irq4, pwm);
        if (irq_id < 0) {
            ZF_LOGE("Unable to register timer irq4");
            return irq_id;
        }
        pwm->t4_irq = irq_id;
        break;

    default:
        break;
    }

    return 0;
}

void pwm_destroy(pwm_t *pwm)
{
    int error;

    /* pre-set INVALID_IRQ_ID before init and do not run if not initialised */
    if (pwm->t0_irq != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&pwm->ops.irq_ops, pwm->t0_irq);
        ZF_LOGE_IF(error, "Unable to un-register timer irq0")
    }
    if (pwm->t4_irq != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&pwm->ops.irq_ops, pwm->t4_irq);
        ZF_LOGE_IF(error, "Unable to un-register timer irq4")
    }

    /* check if pwm_map is NULL and do not run if not initialised */
    if (pwm->pwm_map != NULL) {
        error = pwm_stop(pwm);
        ZF_LOGE_IF(error, "Unable to stop timer pwm")

        ps_pmem_unmap(&pwm->ops, pwm->pmem, (void *) pwm->pwm_map);
    }
}

int pwm_init(pwm_t *pwm, ps_io_ops_t ops, const char *fdt_path, ltimer_callback_fn_t user_cb_fn, void *user_cb_token)
{
    int error;
    ps_fdt_cookie_t *fdt_cookie;

    if (pwm == NULL || fdt_path == NULL) {
        ZF_LOGE("Invalid (null) arguments to pwm timer init");
        return -EINVAL;
    }

    pwm->ops = ops;
    pwm->user_cb_fn = user_cb_fn;
    pwm->user_cb_token = user_cb_token;

    /* these are only valid if the callbacks complete successfully */
    pwm->pwm_map = NULL;    /* if set, implies pmem_region_t valid */
    pwm->t0_irq = PS_INVALID_IRQ_ID;
    pwm->t4_irq = PS_INVALID_IRQ_ID;

    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, fdt_path, &fdt_cookie);
    if (error) {
        ZF_LOGE("Unable to read fdt for pwm timer");
        pwm_destroy(pwm);
        return error;
    }

    error = ps_fdt_walk_registers(&ops.io_fdt, fdt_cookie, pwm_walk_registers, pwm);
    if (error) {
        ZF_LOGE("Unable to walk fdt registers for pwm timer");
        pwm_destroy(pwm);
        return error;
    }

    error = ps_fdt_walk_irqs(&ops.io_fdt, fdt_cookie, pwm_walk_irqs, pwm);
    if (error) {
        ZF_LOGE("Unable to walk fdt irqs for pwm timer");
        pwm_destroy(pwm);
        return error;
    }

    error = ps_fdt_cleanup_cookie(&ops.malloc_ops, fdt_cookie);
    if (error) {
        ZF_LOGE("Unable to free fdt cookie for pwm timer");
        pwm_destroy(pwm);
        return error;
    }

    /* callback section of pwm_t should be set up properly */

    error = pwm_start(pwm);
    if (error) {
        ZF_LOGE("Unable to configure start pwm timer");
        pwm_destroy(pwm);
        return error;
    }

    return 0;
}

int pwm_reset(pwm_t *pwm)
{
    int error;

    error = pwm_stop(pwm);
    if (error) {
        ZF_LOGE("Unable to configure stop pwm timer (reset)");
        return error;
    }

    error = pwm_start(pwm);
    if (error) {
        ZF_LOGE("Unable to configure start pwm timer (reset)");
        return error;
    }

    return 0;
}
