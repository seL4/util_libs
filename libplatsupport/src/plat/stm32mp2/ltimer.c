/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdio.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/plat/timer.h>
#include <platsupport/io.h>

#include "../../ltimer.h"

typedef struct {
    stm32_t stm32_timer;
    irq_id_t irq_id;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ps_io_ops_t ops;
} stm32_ltimer_t;

static pmem_region_t pmems = {
    .type = PMEM_TYPE_DEVICE,
    .base_addr = TIM2_MAP,
    .length = PAGE_SIZE_4K
};

static ps_irq_t irqs = {
    .type = PS_INTERRUPT,
    .irq.number = TIM2_IRQ,
};

static int get_time(void *data, uint64_t *time)
{
    stm32_ltimer_t *stm32_ltimer = data;

    *time = stm32_get_time(&stm32_ltimer->stm32_timer);

    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    uint64_t timeout;
    stm32_ltimer_t *stm32_ltimer = data;
    uint64_t time = stm32_get_time(&stm32_ltimer->stm32_timer);

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        if ((ns - time) / TICK_NS > UINT32_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }
        timeout = ns;
        break;
    case TIMEOUT_RELATIVE:
    case TIMEOUT_PERIODIC:
        if (ns / TICK_NS > UINT32_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }
        timeout = ns + time;
        break;
    }

    return stm32_set_timeout(&stm32_ltimer->stm32_timer, timeout, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    stm32_ltimer_t *stm32_ltimer = data;

    stm32_stop_timer(&stm32_ltimer->stm32_timer);
    stm32_start_timer(&stm32_ltimer->stm32_timer);

    return 0;
}

static void destroy(void *data)
{
    stm32_ltimer_t *stm32_ltimer = data;

    stm32mp2_timer_destroy(&stm32_ltimer->stm32_timer, stm32_ltimer->ops);

    if (stm32_ltimer->irq_id != PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&stm32_ltimer->ops.irq_ops, stm32_ltimer->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }

    if (stm32_ltimer->stm32_timer.hw) {
        ps_pmem_unmap(&stm32_ltimer->ops, pmems, (void *) stm32_ltimer->stm32_timer.hw);
    }

    ps_free(&stm32_ltimer->ops.malloc_ops, sizeof(stm32_ltimer_t), stm32_ltimer);
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    stm32_ltimer_t *stm32_ltimer = data;

    long irq_number = irq->irq.number;
    stm32_t *stm32 = &stm32_ltimer->stm32_timer;
    volatile stm32_regs_t *stm32_regs = stm32->hw;
    uint32_t sr;

    assert(irq_number == irqs.irq.number);

    sr = stm32_regs->sr;

    if (sr & STM32_TIM_SR_UIF) {
        stm32->cnt_h++;
        sr &= ~STM32_TIM_SR_UIF;

        if (stm32_ltimer->user_cb_fn) {
            stm32_ltimer->user_cb_fn(stm32_ltimer->user_cb_token, LTIMER_OVERFLOW_EVENT);
        }
    }

    if (sr & STM32_TIM_SR_CC1IF) {
        if (stm32->periodic_t) {
            stm32->hw->ccr1 += stm32->periodic_t;
        } else {
            stm32_regs->dier &= ~STM32_TIM_DIER_CC1IE;
        }
        sr &= ~STM32_TIM_SR_CC1IF;

        if (stm32_ltimer->user_cb_fn) {
            stm32_ltimer->user_cb_fn(stm32_ltimer->user_cb_token, LTIMER_TIMEOUT_EVENT);
        }
    }

    stm32_regs->sr = sr;

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int rc;
    void *stm32_map_base;

    rc = create_ltimer_simple(ltimer, ops, sizeof(stm32_ltimer_t), get_time,
                              set_timeout, reset, destroy);
    if (rc) {
        ZF_LOGE("ltimer creation failed");
        return rc;
    }

    stm32_ltimer_t *stm32_ltimer = ltimer->data;
    stm32_ltimer->ops = ops;

    stm32_t *stm32 = &stm32_ltimer->stm32_timer;

    stm32_ltimer->user_cb_fn = callback;
    stm32_ltimer->user_cb_token = callback_token;

    stm32_map_base = ps_pmem_map(&ops, pmems, false, PS_MEM_NORMAL);
    if (stm32_map_base == NULL) {
        rc = EINVAL;
        goto err_pmem_map;
    }
    stm32_ltimer->stm32_timer.hw = stm32_map_base;

    stm32_ltimer->callback_data.ltimer = ltimer;
    stm32_ltimer->callback_data.irq_handler = handle_irq;
    stm32_ltimer->callback_data.irq = &irqs;

    stm32_ltimer->irq_id = ps_irq_register(&ops.irq_ops, irqs, handle_irq_wrapper,
                                           &stm32_ltimer->callback_data);
    if (stm32_ltimer->irq_id < 0) {
        rc = EIO;
        goto err_irq_register;
    }

    rc = stm32mp2_timer_init(&stm32_ltimer->stm32_timer, ops);
    if (rc) {
        ZF_LOGE("Failed to init stm32 timeout timer");
        goto err_timer_init;
    }

    return 0;

err_timer_init:
    ps_irq_unregister(&ops.irq_ops, stm32_ltimer->irq_id);

err_irq_register:
    ps_pmem_unmap(&ops, pmems, (void *) stm32_ltimer->stm32_timer.hw);

err_pmem_map:
    ps_free(&ops.malloc_ops, sizeof(stm32_ltimer_t), stm32_ltimer);

    return rc;
}
