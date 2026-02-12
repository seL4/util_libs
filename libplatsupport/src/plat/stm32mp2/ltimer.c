/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include "../../services.h"

#include <platsupport/plat/timer.h>
#include <platsupport/io.h>

#include "../../ltimer.h"

/*
 * We use two dm timers: one to keep track of an absolute time, the other for timeouts.
 */
typedef struct {
    stm32_t stm32_timeout;
    stm32_t stm32_timestamp;
    ps_io_ops_t ops;
} stm32_ltimer_t;

static int get_time(void *data, uint64_t *time)
{
    stm32_ltimer_t *stm32_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    *time = stm32_get_time(&stm32_ltimer->stm32_timestamp);

    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    uint64_t timeout, current_time;
    assert(data != NULL);
    stm32_ltimer_t *stm32_ltimer = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        get_time(data, &current_time);

        if (current_time > ns) {
            printf("Timeout absolute in the past: now %lu, timeout %lu\n", current_time, ns);
            return ETIME;
        }

        timeout = (ns - current_time);
       break;
    case TIMEOUT_PERIODIC:
        timeout = ns;
       break;
    case TIMEOUT_RELATIVE:
         timeout = ns;
         break;
    }

    return stm32_set_timeout(&stm32_ltimer->stm32_timeout, timeout, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    stm32_ltimer_t *stm32_ltimer = data;

    stm32_stop_timer(&stm32_ltimer->stm32_timeout);
    stm32_start_timer(&stm32_ltimer->stm32_timeout);

    return 0;
}

static void destroy(void *data)
{
    assert(data != NULL);

    stm32_ltimer_t *stm32_ltimer = data;
    stm32_destroy(&stm32_ltimer->stm32_timeout);
    stm32_destroy(&stm32_ltimer->stm32_timestamp);
    ps_free(&stm32_ltimer->ops.malloc_ops, sizeof(stm32_ltimer_t), stm32_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int rc;
    void *rcc = NULL;
    ps_io_ops_t* io_ops = &ops;

    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    rc = create_ltimer_simple(ltimer, ops, sizeof(stm32_ltimer_t), get_time,
			      set_timeout, reset, destroy);
    if (rc) {
        ZF_LOGE("ltimer creation failed");
        return rc;
    }

    stm32_ltimer_t *stm32_ltimer = ltimer->data;
    stm32_ltimer->ops = ops;

    /* set up a timer for timeout */
    stm32_config_t stm32_config = {
        .fdt_path = STM32_TIM2_PATH,
        .user_cb_fn = callback,
        .user_cb_token = callback_token,
        .user_cb_event = LTIMER_TIMEOUT_EVENT,
    };

    MAP_IF_NULL(io_ops, RCC_TIM, rcc);
    if (rcc == NULL)
        return -1;

    /* TIM2 RCC */
    RCC_ON(rcc);
    /* TIM3 RCC */
    RCC_ON(rcc+4);

    rc = stm32mp2_timer_init(&stm32_ltimer->stm32_timeout, ops, stm32_config);
    if (rc) {
        ZF_LOGE("Failed to init stm32 timeout timer");
        destroy(&stm32_ltimer);
        return rc;
    }

    /* another for timestamps */
    stm32_config.fdt_path = STM32_TIM3_PATH;
    stm32_config.user_cb_event = LTIMER_OVERFLOW_EVENT;

    rc = stm32mp2_timer_init(&stm32_ltimer->stm32_timestamp, ops, stm32_config);
    if (rc) {
        ZF_LOGE("Failed to init stm32 timestamp timer");
        destroy(&stm32_ltimer);
        return rc;
    }

    rc = stm32_start_timer(&stm32_ltimer->stm32_timestamp);
    if (rc) {
        ZF_LOGE("Failed to start timestamp timer");
        destroy(stm32_ltimer);
        return rc;
    }

    return 0;
}
