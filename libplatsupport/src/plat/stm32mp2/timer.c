/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <utils/util.h>
#include <platsupport/timer.h>
#include <platsupport/fdt.h>
#include <platsupport/plat/timer.h>
#include <utils/frequency.h>

#include "../../ltimer.h"

//debug method
static void print_regs(stm32_t *stm32)
{
    printf("control register 1  >> 0x%08x\n", stm32->hw->cr1);
    printf("control register 2  >> 0x%08x\n", stm32->hw->cr2);
    printf("dier                >> 0x%08x\n", stm32->hw->dier);
    printf("status register     >> 0x%08x\n", stm32->hw->sr);
    printf("egr                 >> 0x%08x\n", stm32->hw->egr);
    printf("ccr1                >> 0x%08x\n", stm32->hw->egr);
    printf("counter             >> 0x%08x\n", stm32->hw->cnt);
    printf("prescaler           >> 0x%08x\n", stm32->hw->psc);
    printf("arr                 >> 0x%08x\n", stm32->hw->arr);
    printf("IPIDR               >> 0x%08x\n", stm32->hw->ipidr);
}

int stm32_start_timer(stm32_t *stm32)
{
    assert(stm32 != NULL);
    assert(stm32->hw  != NULL);

    stm32->hw->sr = 0;
    stm32->hw->cnt = 0;
    stm32->hw->cr1 |= STM32_TIM_CR1_CEN;

    return 0;
}

int stm32_stop_timer(stm32_t *stm32)
{
    if (stm32 == NULL) {
        return EINVAL;
    }

    stm32->hw->cr1 = 0;
    return 0;
}

int stm32_set_timeout(stm32_t *stm32, uint64_t ns, bool periodic)
{
    uint32_t arr;

    if (stm32 == NULL || stm32->hw == NULL) {
        return EINVAL;
    }

    stm32->hw->cr1 &= ~STM32_TIM_CR1_CEN;

    stm32->hw->cnt = 0;
    stm32->hw->arr = ((stm32->freq * ns) / 1000000000UL);
    stm32->hw->dier = STM32_TIM_DIER_UIE;

    stm32->periodic = periodic;

    stm32->hw->cr1 |= STM32_TIM_CR1_CEN;

    return 0;
}

uint64_t stm32_get_time(stm32_t *stm32)
{
    uint64_t cnt;

    if (stm32 == NULL) {
        return EINVAL;
    }

    volatile stm32_regs_t *stm32_regs = stm32->hw;

    if (!stm32->hw->cr1)
        printf("XXX TIMER %s NOT STARTED !!!", stm32->user_cb_event == LTIMER_TIMEOUT_EVENT ? "timeout" : "timestamp");

    cnt = stm32_regs->cnt;

    return (cnt * 1000000000UL) / stm32->freq;
}

static void stm32_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    stm32_t *stm32 = data;
    volatile stm32_regs_t *stm32_regs = stm32->hw;
    uint64_t time;

    time = stm32_get_time(data);

    if (!stm32->periodic) {
        stm32->hw->cr1 &= ~STM32_TIM_CR1_CEN;
        stm32->hw->dier = 0;
    }

    stm32->hw->sr &= ~STM32_TIM_SR_UIF;

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (stm32->user_cb_fn) {
        stm32->user_cb_fn(stm32->user_cb_token, stm32->user_cb_event);
    }
}

void stm32_destroy(stm32_t *stm32)
{
    int error;
    if (stm32->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&stm32->ops.irq_ops, stm32->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (stm32->hw != NULL) {
        stm32_stop_timer(stm32);
        ps_pmem_unmap(&stm32->ops, stm32->pmem, (void *) stm32->hw);
    }
}

static int irq_index_walker(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    stm32_t *stm32 = token;

    if (STM32_IRQ_CHOICE == curr_num) {
        irq_id_t registered_id = ps_irq_register(&stm32->ops.irq_ops, irq, stm32_handle_irq, stm32);
        if (registered_id >= 0) {
            stm32->irq_id = registered_id;
            stm32->irq = irq;
        } else {
	    /* Bail on error */
	    return registered_id;
        }
    }

    return 0;
}

int stm32mp2_timer_init(stm32_t *stm32, ps_io_ops_t ops, stm32_config_t config)
{
    int error;

    if (stm32 == NULL) {
        ZF_LOGE("stm32 cannot be null");
        return EINVAL;
    }

    stm32->ops = ops;
    stm32->user_cb_fn = config.user_cb_fn;
    stm32->user_cb_token = config.user_cb_token;
    stm32->user_cb_event = config.user_cb_event;

    ps_fdt_cookie_t *cookie = NULL;
    error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, config.fdt_path, &cookie);
    if (error) {
        ZF_LOGE("Failed to read path (%d, %s)", error, config.fdt_path);
        return error;
    }

    stm32->stm32_map_base = ps_fdt_index_map_register(&ops, cookie, STM32_REG_CHOICE, &stm32->pmem);
    if (stm32->stm32_map_base == NULL) {
        ZF_LOGE("Failed to map registers");
        return ENODEV;
    }

    irq_id_t irq_id = ps_fdt_walk_irqs(&ops.io_fdt, cookie, irq_index_walker, stm32);
    if (irq_id) {
        ZF_LOGE("timer failed to register irqs (%d)", irq_id);
        return irq_id;
    }

    error = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
    if (error) {
        ZF_LOGE("timer to clean up cookie (%d)", error);
        return error;;
    }

    stm32->hw = stm32->stm32_map_base;

    uint64_t apb1 = 200000000UL;
    uint32_t psc = 1;

    stm32->hw->egr = STM32_TIM_EGR_UG;
    stm32->hw->sr = 0;
    stm32->freq = (apb1/psc);

    stm32->hw->psc = psc - 1;

    return 0;
}


