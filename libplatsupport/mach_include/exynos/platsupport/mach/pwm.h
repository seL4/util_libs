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

#include <stdint.h>

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
} pwm_config_t;

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

static UNUSED timer_properties_t pwm_properties = {
    .upcounter = false,
    .bit_width = 32,
    .irqs = 2,
    .periodic_timeouts = true,
    .relative_timeouts = true,
    .absolute_timeouts = false,
    .timeouts = true,
};

int pwm_start(pwm_t *pwm);
int pwm_stop(pwm_t *pwm);
void pwm_handle_irq(pwm_t *pwm, uint32_t irq);
uint64_t pwm_get_time(pwm_t *pwm);
int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic);
int pwm_init(pwm_t *pwm, pwm_config_t config);
