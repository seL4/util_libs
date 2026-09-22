/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/ltimer.h>
#include <platsupport/timer.h>

/* Input clock */
#define TIM2_HZ 200000000UL
#define TICK_NS 5000UL

/* RCC TIM2 basic timer */
#define RCC_PADDR      0x44200000
#define RCC_TIM2_OFF   0x704
#define RCC_TIM2_PADDR (RCC_PADDR + RCC_TIM2_OFF)
#define RCC_TIM2_SIZE  4
#define TIM2_IRQ       137
#define TIM2_MAP       0x40000000

typedef struct stm32_regs {
    uint32_t cr1;
    uint32_t cr2;
    uint32_t smcr;
    uint32_t dier;
    uint32_t sr;
    uint32_t egr;
    uint32_t ccmr1;
    uint32_t ccmr2;
    uint32_t ccer;
    uint32_t cnt;
    uint32_t psc;
    uint32_t arr;
    uint32_t rcr; /* n/a for TIM2 */
    uint32_t ccr1;
    uint32_t ccr2;
    uint32_t ccr3;
    uint32_t ccr4;
} stm32_regs_t;

#define STM32_TIM_CCER_CC1E   BIT(0)

#define STM32_TIM_CR1_CEN     BIT(0)
#define STM32_TIM_CR1_UDIS    BIT(1)
#define STM32_TIM_CR1_URS     BIT(1)
#define STM32_TIM_CR1_OPM     BIT(6)
#define STM32_TIM_CR1_ARPE    BIT(7)

#define STM32_TIM_EGR_UG      BIT(0)

#define STM32_TIM_DIER_UIE    BIT(0)
#define STM32_TIM_DIER_CC1IE  BIT(1)

#define STM32_TIM_SR_UIF      BIT(0)
#define STM32_TIM_SR_CC1IF    BIT(1)

typedef struct {
    volatile stm32_regs_t *hw;
    uint32_t periodic_t;
    uint32_t cnt_h; /* keep overflow for get_time */
    void *rcc;
} stm32_t;

int stm32mp2_timer_init(stm32_t *stm32, ps_io_ops_t ops);
void stm32mp2_timer_destroy(stm32_t *stm32, ps_io_ops_t ops);
void stm32_timer_reset(stm32_t *stm32);
uint64_t stm32_get_time(stm32_t *stm32);
int stm32_set_timeout(stm32_t *dmt, uint64_t ns, bool periodic);
int stm32_start_timer(stm32_t *stm32);
int stm32_stop_timer(stm32_t *stm32);
