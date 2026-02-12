/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/fdt.h>
#include <platsupport/ltimer.h>
#include <platsupport/timer.h>

#define STM32_REG_CHOICE 0
#define STM32_IRQ_CHOICE 0

#define RCC_PADDR      0x44200000

/* TIM2/TIM3 basic timers */
#define RCC_TIM_OFF        0x704
#define RCC_TIM_PADDR      (RCC_PADDR + RCC_TIM_OFF)
#define RCC_TIM_SIZE       8

#define STM32_TIM2_PATH "/soc@0/bus@42080000/timer@40000000"
#define STM32_TIM3_PATH "/soc@0/bus@42080000/timer@40010000"

#define RCC_ON(rcc)    (*(volatile uint32_t *)(rcc) = 6)

#define RCC_APB1DIVR_OFF  0x4B4
#define RCC_APB1DIVR_PADDR      (RCC_PADDR + RCC_APB1DIVR_OFF)
#define RCC_APB1DIVR_SIZE       4

typedef struct stm32_regs {
    uint32_t cr1;  /* 0 */
    uint32_t cr2;  /* 4 */
    uint32_t smcr; /* 8 */
    uint32_t dier; /* c */
    uint32_t sr; /* 10 */
    uint32_t egr; /* 14 */
    uint32_t ccmr1; /* 18 */
    uint32_t ccmr2; /* 1c */
    uint32_t ccer; /* 20 */
    uint32_t cnt; /* 24 */
    uint32_t psc; /* 28 */
    uint32_t arr; /* 2c */
    uint32_t ccr1;
    uint32_t ccr2;
    uint32_t ccr3;
    uint32_t ccr4;
    uint32_t bdtr;
    uint32_t dcr;
    uint32_t dmar;
    uint32_t tisel;
    uint32_t pad[0xEA];
    uint32_t ipidr;
} stm32_regs_t;

#define STM32_TIM_CR1_CEN     BIT(0)
#define STM32_TIM_CR1_UDIS    BIT(1)
#define STM32_TIM_CR1_URS     BIT(1)
#define STM32_TIM_CR1_OPM     BIT(6)
#define STM32_TIM_CR1_ARPE    BIT(7)

#define STM32_TIM_EGR_UG      BIT(0)

#define STM32_TIM_DIER_UIE    BIT(0)
#define STM32_TIM_DIER_CC1IE  BIT(1)

#define STM32_TIM_SR_UIF      BIT(0)

typedef struct {
    /* set in init */
    ps_io_ops_t ops;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;
    volatile stm32_regs_t *hw;
    void *stm32_map_base;
    pmem_region_t pmem;
    ps_irq_t irq;
    irq_id_t irq_id;
    bool periodic;
    uint64_t freq;
} stm32_t;

typedef struct {
    /* set in init */
    const char *fdt_path;
    ltimer_callback_fn_t user_cb_fn;
    void *user_cb_token;
    ltimer_event_t user_cb_event;  /* what are we being used for? */
} stm32_config_t;

int stm32mp2_timer_init(stm32_t *stm32, ps_io_ops_t ops, stm32_config_t config);
void stm32_timer_reset(stm32_t *stm32);
uint64_t stm32_get_time(stm32_t *stm32);
int stm32_set_timeout(stm32_t *dmt, uint64_t ns, bool periodic);
void stm32_destroy(stm32_t *stm32);
int stm32_start_timer(stm32_t *stm32);
int stm32_stop_timer(stm32_t *stm32);

