/*
 * Copyright 2019, Data61
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

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

/* The input frequence is 0.5 of the CPU frequency which is 1GHz by default */
#define PWM_INPUT_FREQ (500*1000*1000)

#define PWM0_PATH "/soc/pwm@10020000"
#define PWM1_PATH "/soc/pwm@10021000"

/* There is only one register map available in device tree */
#define PWM_REG_CHOICE 0
/* There are four interrupts for matches available, use the first one */
#define PWM_IRQ_CHOICE 0

/**
 * When used in UPCOUNTER, pwm_handle_irq needs to be called on each interrupt
 * to handle the overflow. This device doesn't seem to have a way to tell that
 * an overflow happened other than by the delivery of an IRQ. This means that
 * if pwm_get_time is called before pwm_handle_irq the time could be incorrect.
 */
typedef enum PWM_MODE {
    TIMEOUT,
    UPCOUNTER,
} pwm_mode_t;

typedef struct {
    char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    pwm_mode_t mode;
} pwm_config_t;

/* Memory map for pwm */
struct pwm_map {
    uint32_t pwmcfg;
    uint32_t res0;
    uint32_t pwmcount;
    uint32_t res1;
    uint32_t pwms;
    uint32_t res2;
    uint32_t res3;
    uint32_t res4;
    uint32_t pwmcmp0;
    uint32_t pwmcmp1;
    uint32_t pwmcmp2;
    uint32_t pwmcmp3;
};

typedef struct pwm {
    /* set in init */
    ps_io_ops_t ops;
    pwm_mode_t mode;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;

    /* set in fdt helper */
    volatile struct pwm_map *pwm_map;
    pmem_region_t pmem;
    irq_id_t irq_id;

    /* set in setup */
    uint64_t time_h;
} pwm_t;

static UNUSED timer_properties_t pwm_properties = {
    .upcounter = true,
    .bit_width = 31,
    .irqs = 4,
    .periodic_timeouts = true,
    .relative_timeouts = true,
    .absolute_timeouts = false,
    .timeouts = true,
};

void pwm_start(pwm_t *pwm);
void pwm_stop(pwm_t *pwm);
uint64_t pwm_get_time(pwm_t *pwm);
int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic);
int pwm_init(pwm_t *pwm, ps_io_ops_t ops, pwm_config_t config);
void pwm_destroy(pwm_t *pwm);
