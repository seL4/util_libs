/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Use 32 bit TIM2 general timer to handle 64 bit timestamp with ARR
   overflow in up-counting mode. CNT is reset at earch overflow.
   32 bit timeout interrupt is supported with Channel 1 output compare. */

#include <stdio.h>
#include <errno.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>
#include <utils/frequency.h>

#include "../../services.h"
#include "../../ltimer.h"

static void print_regs(stm32_t *stm32)
{
    printf("control register 1  >> 0x%08x\n", stm32->hw->cr1);
    printf("control register 2  >> 0x%08x\n", stm32->hw->cr2);
    printf("dier                >> 0x%08x\n", stm32->hw->dier);
    printf("status register     >> 0x%08x\n", stm32->hw->sr);
    printf("egr                 >> 0x%08x\n", stm32->hw->egr);
    printf("ccr1                >> 0x%08x\n", stm32->hw->ccr1);
    printf("counter             >> 0x%08x\n", stm32->hw->cnt);
    printf("prescaler           >> 0x%08x\n", stm32->hw->psc);
    printf("arr                 >> 0x%08x\n", stm32->hw->arr);
}

int stm32_start_timer(stm32_t *stm32)
{
    stm32->hw->cnt = 0;
    stm32->cnt_h = 0;

    stm32->hw->cr1 |= STM32_TIM_CR1_CEN;

    return 0;
}

int stm32_stop_timer(stm32_t *stm32)
{
    stm32->hw->cr1 &= ~STM32_TIM_CR1_CEN;
    stm32->hw->dier &= ~STM32_TIM_DIER_CC1IE;

    return 0;
}

int stm32_set_timeout(stm32_t *stm32, uint64_t timeout, bool periodic)
{
    uint32_t target = timeout / TICK_NS;

    stm32->hw->ccr1 = target;
    stm32->hw->sr &= ~STM32_TIM_SR_CC1IF;
    stm32->hw->dier |= STM32_TIM_DIER_CC1IE;
    stm32->periodic_t = periodic ? target : 0UL;

    return 0;
}

uint64_t stm32_get_time(stm32_t *stm32)
{
    uint32_t cnt = stm32->hw->cnt;
    uint32_t hi = stm32->cnt_h;

    /* Check for pending overflow. */
    if (stm32->hw->sr & STM32_TIM_SR_UIF) {
        hi++;
        /* In case it happened after the last read */
        cnt = stm32->hw->cnt;
    }

    uint64_t cnt2 = ((uint64_t)hi << 32) | cnt;

    return (cnt2 * TICK_NS);
}

void enable_rcc(void *rcc)
{
    volatile uint32_t *rcc_cfgr = rcc;

    /* reset and enable clock */
    *rcc_cfgr = BIT(0); /* assert */
    *rcc_cfgr = BIT(2) | BIT(1); /* de-assert and enable */

    /* introduce read-back to wait the clock to propagate */
    while (!(*rcc_cfgr & (BIT(2) | BIT(1))));
}

void stm32mp2_timer_destroy(stm32_t *stm32_timer,  ps_io_ops_t ops)
{
    if (stm32_timer->rcc == NULL) {
        return;
    }

    stm32_stop_timer(stm32_timer);
    ps_io_unmap(&ops.io_mapper, stm32_timer->rcc, RCC_TIM2_SIZE);
}

int stm32mp2_timer_init(stm32_t *stm32_timer, ps_io_ops_t ops)
{
    ps_io_ops_t *io_ops = &ops;

    /* TIM2 RCC */
    MAP_IF_NULL(io_ops, RCC_TIM2, stm32_timer->rcc);
    if (stm32_timer->rcc == NULL) {
        ZF_LOGE("Failed map TIM2 RCC");
        return EINVAL;
    }

    enable_rcc(stm32_timer->rcc);

    stm32_timer->hw->psc = (uint16_t)((TICK_NS * TIM2_HZ) / NS_IN_S) - 1;
    stm32_timer->hw->arr = UINT32_MAX;
    stm32_timer->hw->egr  = STM32_TIM_EGR_UG;    /* force update event */
    stm32_timer->hw->dier = STM32_TIM_DIER_UIE;  /* enable interrupt for overflow */
    stm32_timer->hw->ccer = STM32_TIM_CCER_CC1E; /* enable channel1 output */

    stm32_start_timer(stm32_timer);

    return 0;
}
