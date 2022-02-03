/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/system_timer.h>
#include <platsupport/mach/system_timer.h>
#include <utils/util.h>

#include "../../ltimer.h"

typedef struct {
    bcm_system_timer_t system_timer;
    ps_io_ops_t ops;
} bcm_ltimer_t;

static int bcm_ltimer_get_time(void *data, uint64_t *time)
{
    if (!data || !time) {
        return EINVAL;
    }

    bcm_ltimer_t *timer = data;

    return bcm_system_timer_get_time(&timer->system_timer, time);
}

static int bcm_ltimer_set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    if (!data) {
        return EINVAL;
    }
    bcm_ltimer_t *timer = data;

    return bcm_system_timer_set_timeout(&timer->system_timer, ns, type);
}

static int bcm_ltimer_reset(void *data)
{
    if (!data) {
        return EINVAL;
    }
    bcm_ltimer_t *ltimer = data;

    return bcm_system_timer_reset(&ltimer->system_timer);
}

static void bcm_ltimer_destroy(void *data)
{
    if (!data) {
        return;
    }
    bcm_ltimer_t *timer = data;

    bcm_system_timer_destroy(&timer->system_timer);
    ps_free(&timer->ops.malloc_ops, sizeof(bcm_ltimer_t), timer);
}

static int bcm_ltimer_create(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (!ltimer) {
        return EINVAL;
    }

    return create_ltimer_simple(ltimer,
                                ops,
                                sizeof(bcm_ltimer_t),
                                bcm_ltimer_get_time,
                                bcm_ltimer_set_timeout,
                                bcm_ltimer_reset,
                                bcm_ltimer_destroy);
}

int ltimer_default_init(ltimer_t *ltimer,
                        ps_io_ops_t ops,
                        ltimer_callback_fn_t callback,
                        void *callback_token)
{
    int rc;

    if (!ltimer) {
        return EINVAL;
    }

    rc = bcm_ltimer_create(ltimer, ops);
    if (rc) {
        ZF_LOGE("ltimer creation failed");
        return rc;
    }
    bcm_ltimer_t *timer = ltimer->data;
    timer->ops = ops;

    bcm_system_timer_config_t config = {
        .channel = BCM_SYSTEM_TIMER_CHANNEL,
        .frequency = BCM_SYSTEM_TIMER_FREQ,
        .fdt_path = BCM_SYSTEM_TIMER_FDT_PATH,
        .fdt_reg_choice = BCM_SYSTEM_TIMER_REG_CHOICE,
        .fdt_irq_choice = BCM_SYSTEM_TIMER_IRQ_CHOICE,
    };

    rc = bcm_system_timer_init(&timer->system_timer,
                               ops,
                               callback,
                               callback_token,
                               config);
    if (rc) {
        ZF_LOGE("system timer initialisation failed");
        ps_free(&timer->ops.malloc_ops, sizeof(bcm_system_timer_t), timer);
    }

    return rc;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
