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
/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/pwm.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    pwm_t pwm;
    ps_io_ops_t ops;
} pwm_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    pwm_ltimer_t *pwm_ltimer = data;
    *time = pwm_get_time(&pwm_ltimer->pwm);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    pwm_ltimer_t *pwm_ltimer = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = pwm_get_time(&pwm_ltimer->pwm);
        if (time >= ns) {
            return ETIME;
        }
        return pwm_set_timeout(&pwm_ltimer->pwm, ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return pwm_set_timeout(&pwm_ltimer->pwm, ns, false);
    case TIMEOUT_PERIODIC:
        return pwm_set_timeout(&pwm_ltimer->pwm, ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    pwm_ltimer_t *pwm_ltimer = data;
    pwm_reset(&pwm_ltimer->pwm);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    pwm_ltimer_t *pwm_ltimer = data;
    pwm_destroy(&pwm_ltimer->pwm);
    ps_free(&pwm_ltimer->ops.malloc_ops, sizeof(pwm_ltimer_t), pwm_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    assert(ltimer != NULL);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;
    ltimer->get_nth_irq = NULL;
    ltimer->get_nth_pmem = NULL;
    ltimer->get_num_irqs = NULL;
    ltimer->get_num_pmems = NULL;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(pwm_ltimer_t), &ltimer->data);
    if (error) {
        ZF_LOGE("Unable to allocate ltimer data");
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    error = create_ltimer(ltimer, ops);
    if (error) {
        ZF_LOGE("Unable to create ltimer");
        return error;
    }

    pwm_ltimer_t *pwm_ltimer = ltimer->data;
    pwm_ltimer->ops = ops;

    error = pwm_init(&pwm_ltimer->pwm, ops, PWM_TIMER_PATH, callback, callback_token);
    if (error) {
        ZF_LOGE("Unable to create pwm for ltimer");
        return error;
    }

    /* success! */
    return 0;
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t UNUSED *ltimer, ps_io_ops_t UNUSED ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
