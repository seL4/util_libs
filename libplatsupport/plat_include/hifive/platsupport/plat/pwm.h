/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/timer.h>

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

/* The input frequence is 0.5 of the CPU frequency which is 1GHz by default */
#define PWM_INPUT_FREQ (500*1000*1000)
#define PWM0_PADDR   0x10020000
#define PWM1_PADDR   0x10021000

#define PWM0_INTERRUPT0 42
#define PWM0_INTERRUPT1 43
#define PWM0_INTERRUPT2 44
#define PWM0_INTERRUPT3 45

#define PWM1_INTERRUPT0 46
#define PWM1_INTERRUPT1 47
#define PWM1_INTERRUPT2 48
#define PWM1_INTERRUPT3 49

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
    /* vaddr pwm is mapped to */
    void *vaddr;
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
    volatile struct pwm_map *pwm_map;
    uint64_t time_h;
    pwm_mode_t mode;
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

int pwm_start(pwm_t *pwm);
int pwm_stop(pwm_t *pwm);
void pwm_handle_irq(pwm_t *pwm, uint32_t irq);
uint64_t pwm_get_time(pwm_t *pwm);
int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic);
int pwm_init(pwm_t *pwm, pwm_config_t config);
