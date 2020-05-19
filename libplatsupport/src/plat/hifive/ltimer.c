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
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/pwm.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#include "../../ltimer.h"

/* Implementation of a logical timer for HiFive Unleashed platform.
 *
 * We use two pwms: one for the time and the other for timeouts.
 *
 * De-multiplex common interface operations into two separate timers.
 */

typedef struct {
    ps_io_ops_t ops;
    pwm_t pwm_counter;
    pwm_t pwm_timeout;
} hifive_timers_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    hifive_timers_t *timers = data;

    *time = pwm_get_time(&timers->pwm_counter);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    hifive_timers_t *timers = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = pwm_get_time(&timers->pwm_counter);
        if (time >= ns) {
            return ETIME;
        }
        return pwm_set_timeout(&timers->pwm_timeout, ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return pwm_set_timeout(&timers->pwm_timeout, ns, false);
    case TIMEOUT_PERIODIC:
        return pwm_set_timeout(&timers->pwm_timeout, ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    hifive_timers_t *timers = data;
    pwm_stop(&timers->pwm_counter);
    pwm_stop(&timers->pwm_timeout);
    pwm_start(&timers->pwm_counter);
    pwm_start(&timers->pwm_timeout);
    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    hifive_timers_t *timers = data;
    pwm_destroy(&timers->pwm_counter);
    pwm_destroy(&timers->pwm_timeout);
    ps_free(&timers->ops.malloc_ops, sizeof(hifive_timers_t), timers);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    error = create_ltimer_simple(
                ltimer, ops, sizeof(hifive_timers_t),
                get_time, set_timeout, reset, destroy
            );
    if (error) {
        /* failed to create, no need to destroy */
        ZF_LOGE("Failed to create ltimer using simple helper");
        return error;
    }

    hifive_timers_t *timers = ltimer->data;
    timers->ops = ops;

    pwm_config_t config = {
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .fdt_path = PWM0_PATH,
        .mode = UPCOUNTER,
    };

    error = pwm_init(&timers->pwm_counter, ops, config);
    if (error) {
        ZF_LOGE("Failed to init pwm timer (UPCOUNTER)");
        destroy(timers);
        return error;
    }

    config.fdt_path = PWM1_PATH;
    config.mode = TIMEOUT;

    error = pwm_init(&timers->pwm_timeout, ops, config);
    if (error) {
        ZF_LOGE("Failed to init pwm timer (TIMEOUT)");
        destroy(timers);
        return error;
    }

    error = reset(timers);
    if (error) {
        ZF_LOGE("Failed to reset pwm timers during init");
        destroy(timers);
        return error;
    }

    /* success! */
    return 0;
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
