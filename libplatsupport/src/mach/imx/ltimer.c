/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Implementation of a logical timer for imx platforms
 *
 * Currently all imx platforms use some combination of GPT and EPIT timers to provide ltimer functionality. See platform specific timer.h for details.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <platsupport/irq.h>

#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    imx_timers_t timers;
    bool timestamp_initialised;
    bool timeout_initialised;
    ps_io_ops_t ops;
} imx_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    imx_ltimer_t *imx_ltimer = data;
    *time = imx_get_time(&imx_ltimer->timers);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = imx_get_time(&imx_ltimer->timers);
        if (ns < current_time) {
            return ETIME;
        }
        ns -= current_time;
    }

    return imx_set_timeout(&imx_ltimer->timers, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    imx_ltimer_t *imx_ltimer = data;

    /* reset the timers */
    imx_stop_timeout(&imx_ltimer->timers);
    imx_stop_timestamp(&imx_ltimer->timers);
    imx_start_timestamp(&imx_ltimer->timers);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    imx_ltimer_t *imx_ltimer = data;

    if (imx_ltimer->timestamp_initialised) {
        imx_stop_timestamp(&imx_ltimer->timers);
        ZF_LOGF_IF(imx_destroy_timestamp(&imx_ltimer->timers), "Failed to destroy the timestamp timer");
    }

    if (imx_ltimer->timeout_initialised) {
        imx_stop_timeout(&imx_ltimer->timers);
        ZF_LOGF_IF(imx_destroy_timeout(&imx_ltimer->timers), "Failed to destroy the timeout timer");
    }

    ps_free(&imx_ltimer->ops.malloc_ops, sizeof(*imx_ltimer), imx_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    assert(ltimer != NULL);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(imx_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    imx_ltimer_t *imx_ltimer = ltimer->data;

    imx_ltimer->ops = ops;

    error = imx_init_timestamp(&imx_ltimer->timers, ops, callback, callback_token);
    if (error) {
        ZF_LOGE("Failed to init timestamp timer");
        ltimer_destroy(ltimer);
        return error;
    }

    imx_ltimer->timestamp_initialised = true;

    imx_start_timestamp(&imx_ltimer->timers);

    error = imx_init_timeout(&imx_ltimer->timers, ops, callback, callback_token);
    if (error) {
        ZF_LOGE("Failed to init timeout timer");
        ltimer_destroy(ltimer);
        return error;
    }
    imx_ltimer->timeout_initialised = true;

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
