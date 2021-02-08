/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/gpt.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

typedef struct {
    gpt_t abs_gpt;
    gpt_t rel_gpt;
    ps_io_ops_t ops;
} omap_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    omap_ltimer_t *omap_ltimer = data;
    *time = abs_gpt_get_time(&omap_ltimer->abs_gpt);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time = abs_gpt_get_time(&omap_ltimer->abs_gpt);
        if (ns <= time) {
            return ETIME;
        }
        ns -= time;
    }

    if (ns >= gpt_get_max()) {
        if (type == TIMEOUT_PERIODIC) {
            ZF_LOGW("Timeout too big for periodic timeout on this platform");
            return EINVAL;
        } else {
            /* cap it, caller can deal with earlier interrupts */
            ns = gpt_get_max();
        }
    }

    return rel_gpt_set_timeout(&omap_ltimer->rel_gpt, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;

    gpt_stop(&omap_ltimer->abs_gpt);
    gpt_stop(&omap_ltimer->rel_gpt);

    gpt_start(&omap_ltimer->abs_gpt);
    gpt_start(&omap_ltimer->rel_gpt);

    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);
    omap_ltimer_t *omap_ltimer = data;
    gpt_destroy(&omap_ltimer->abs_gpt);
    gpt_destroy(&omap_ltimer->rel_gpt);
    ps_free(&omap_ltimer->ops.malloc_ops, sizeof(omap_ltimer_t), omap_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error;

    if (ltimer == NULL) {
        return EINVAL;
    }

    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;
    ltimer->get_num_pmems = NULL;
    ltimer->get_num_irqs = NULL;
    ltimer->get_nth_pmem = NULL;
    ltimer->get_nth_irq = NULL;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(omap_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    omap_ltimer_t *omap_ltimer = ltimer->data;
    omap_ltimer->ops = ops;

    error = gpt_create(&omap_ltimer->abs_gpt, ops, GPT1_DEVICE_PATH, callback, callback_token);
    if (error) {
        ZF_LOGE("Failed to create abs gpt timer");
        return error;
    }

    error = gpt_create(&omap_ltimer->rel_gpt, ops, GPT2_DEVICE_PATH, callback, callback_token);
    if (error) {
        ZF_LOGE("Failed to create rel gpt timer");
        return error;
    }

    /* setup gpt */
    gpt_config_t config = {
        .prescaler = 1,
    };

    /* intitialise gpt for getting the time */
    error = abs_gpt_init(&omap_ltimer->abs_gpt, config);
    if (error) {
        ZF_LOGE("Failed to init gpt");
        destroy(ltimer->data);
        return error;
    }

    error = rel_gpt_init(&omap_ltimer->rel_gpt, config);
    if (error) {
        destroy(ltimer->data);
        ZF_LOGE("Failed to init gpt");
        return error;
    }

    gpt_start(&omap_ltimer->abs_gpt);
    gpt_start(&omap_ltimer->rel_gpt);

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
