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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/pwm.h>

#include "../../ltimer.h"

#define PWMSCALE_MASK MASK(4)
#define PWMSTICKY BIT(8)
#define PWMZEROCMP BIT(9)
#define PWMENALWAYS  BIT(12)
#define PWMENONESHOT BIT(13)
#define PWMCMP0IP BIT(28)
#define PWMCMP1IP BIT(29)
#define PWMCMP2IP BIT(30)
#define PWMCMP3IP BIT(31)
#define PWMCMP_WIDTH 16
#define PWMCMP_MASK MASK(PWMCMP_WIDTH)

/* Largest timeout that can be programmed */
#define MAX_TIMEOUT_NS (BIT(31) * (NS_IN_S / PWM_INPUT_FREQ))


void pwm_start(pwm_t *pwm)
{
    assert(pwm != NULL && pwm->pwm_map != NULL);
    pwm->pwm_map->pwmcfg |= PWMENALWAYS;
}

void pwm_stop(pwm_t *pwm)
{
    assert(pwm != NULL && pwm->pwm_map != NULL);
    /* Disable timer. */
    pwm->pwm_map->pwmcmp0 = PWMCMP_MASK;
    pwm->pwm_map->pwmcfg &= ~(PWMENALWAYS | PWMENONESHOT | PWMCMP0IP | PWMCMP1IP | PWMCMP2IP | PWMCMP3IP);
    pwm->pwm_map->pwmcount = 0;
}

int pwm_set_timeout(pwm_t *pwm, uint64_t ns, bool periodic)
{
    if (pwm->mode == UPCOUNTER) {
        ZF_LOGE("pwm is in UPCOUNTER mode and doesn't support setting timeouts.");
        return -1;
    }
    // Clear whatever state the timer is in.
    pwm_stop(pwm);
    size_t num_ticks = ns / (NS_IN_S / PWM_INPUT_FREQ);
    if (num_ticks > MAX_TIMEOUT_NS) {
        ZF_LOGE("Cannot program a timeout larget than %ld ns", MAX_TIMEOUT_NS);
        return -1;
    }

    /* We calculate the prescale by dividing the number of ticks we need
     * by the width of the comparison register. The remainder is how much
     * we want to prescale by, however the prescale value needs to be a
     * power of 2 so we take the log2() and then increment it by 1 if it
     * would otherwise be too low.
     */
    size_t prescale = num_ticks >> (PWMCMP_WIDTH);
    size_t base_2 = LOG_BASE_2(prescale);
    if (BIT(base_2) < prescale) {
        base_2++;
    }
    assert(prescale < BIT(PWMSCALE_MASK));

    /* There will be a loss of resolution by this shift.
     * We add 1 so that we sleep for at least as long as needed.
     */
    pwm->pwm_map->pwmcmp0 = (num_ticks >> base_2) + 1;
    /* assert we didn't overflow... */
    assert((num_ticks >> base_2) + 1);
    /* Reset the counter mode and prescaler, for some reason this doesn't work in pwm_stop */
    pwm->pwm_map->pwmcfg &= ~(PWMSCALE_MASK);
    if (periodic) {
        pwm->pwm_map->pwmcfg |= PWMENALWAYS | (base_2 & PWMSCALE_MASK);
    } else {
        pwm->pwm_map->pwmcfg |= PWMENONESHOT | (base_2 & PWMSCALE_MASK);
    }

    return 0;
}

static void pwm_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    pwm_t *pwm = data;
    if (pwm->mode == UPCOUNTER) {
        pwm->time_h++;
    }
    pwm->pwm_map->pwmcfg &= ~(PWMCMP0IP | PWMCMP1IP | PWMCMP2IP | PWMCMP3IP);
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (pwm->user_cb_fn) {
        pwm->user_cb_fn(
            pwm->user_cb_token,
            pwm->mode == UPCOUNTER ? LTIMER_OVERFLOW_EVENT : LTIMER_TIMEOUT_EVENT
        );
    }
}

uint64_t pwm_get_time(pwm_t *pwm)
{
    /* Include unhandled interrupt. */
    if (pwm->pwm_map->pwmcfg & PWMCMP0IP) {
        pwm->time_h++;
        pwm->pwm_map->pwmcfg &= ~PWMCMP0IP;
    }

    uint64_t num_ticks = (pwm->time_h * (PWMCMP_MASK << PWMSCALE_MASK) + pwm->pwm_map->pwmcount);
    uint64_t time = num_ticks * (NS_IN_S / PWM_INPUT_FREQ);
    return time;
}

static void pwm_setup(pwm_t *pwm)
{
    assert(pwm != NULL);
    pwm->time_h = 0;
    uint8_t scale = 0;
    if (pwm->mode == UPCOUNTER) {
        scale = PWMSCALE_MASK;
    }
    pwm->pwm_map->pwmcmp0 = PWMCMP_MASK;
    pwm->pwm_map->pwmcmp1 = PWMCMP_MASK;
    pwm->pwm_map->pwmcmp2 = PWMCMP_MASK;
    pwm->pwm_map->pwmcmp3 = PWMCMP_MASK;
    pwm->pwm_map->pwmcfg = (scale & PWMSCALE_MASK) | PWMZEROCMP | PWMSTICKY;
}

void pwm_destroy(pwm_t *pwm)
{
    int error;
    if (pwm->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&pwm->ops.irq_ops, pwm->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (pwm->pwm_map != NULL) {
        pwm_stop(pwm);
        ps_pmem_unmap(&pwm->ops, pwm->pmem, (void *) pwm->pwm_map);
    }
}

int pwm_init(pwm_t *pwm, ps_io_ops_t ops, pwm_config_t config)
{
    int error;

    if (pwm == NULL) {
        return EINVAL;
    }

    /* Set up pwm */
    pwm->ops = ops;
    pwm->mode = config.mode;
    pwm->user_cb_fn = config.user_cb_fn;
    pwm->user_cb_token = config.user_cb_token;

    error = helper_fdt_alloc_simple(
                &ops, config.fdt_path,
                PWM_REG_CHOICE, PWM_IRQ_CHOICE,
                (void *) &pwm->pwm_map, &pwm->pmem, &pwm->irq_id,
                pwm_handle_irq, pwm
            );
    if (error) {
        ZF_LOGE("Failed to init pwm timer using simple helper");
        pwm_destroy(pwm);
        return error;
    }

    pwm_setup(pwm);

    return 0;
}
