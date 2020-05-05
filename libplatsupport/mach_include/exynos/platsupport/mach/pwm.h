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

#pragma once

#include <platsupport/timer.h>
#include <platsupport/plat/pwm.h>
#include <platsupport/io.h>
#include <platsupport/pmem.h>
#include <platsupport/ltimer.h>

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

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
    /* set upon entering init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;

    /* set during init callbacks */
    volatile struct pwm_map *pwm_map;
    pmem_region_t pmem;                 /* mapping for pwm_map */
    irq_id_t t0_irq;                    /* irq for timer 0 */
    irq_id_t t4_irq;

    /* set during device start */
    uint64_t time_h;                   /* track overflows for get_time */
} pwm_t;

static UNUSED timer_properties_t pwm_properties = {
    .upcounter = false,
    .bit_width = 32,
    .irqs = 2,
    .periodic_timeouts = true,
    .relative_timeouts = true,
    .absolute_timeouts = false,
    .timeouts = true,
};

uint64_t pwm_get_time(pwm_t *pwm);
int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic);
int pwm_init(pwm_t *pwm, ps_io_ops_t ops, char *fdt_path, ltimer_callback_fn_t user_cb_fn, void *user_cb_token);
void pwm_destroy(pwm_t *pwm);
int pwm_reset(pwm_t *pwm);
