/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/timer.h>

#define TIMER_BASE      0xc1100000
#define TIMER_MAP_BASE  0xc1109000

#define TIMER_REG_START   0x2650    // TIMER_MUX

#define TIMER_E_INPUT_CLK 8
#define TIMER_D_INPUT_CLK 6
#define TIMER_C_INPUT_CLK 4
#define TIMER_B_INPUT_CLK 2
#define TIMER_A_INPUT_CLK 0

#define TIMER_D_EN      BIT(19)
#define TIMER_C_EN      BIT(18)
#define TIMER_B_EN      BIT(17)
#define TIMER_A_EN      BIT(16)
#define TIMER_D_MODE    BIT(15)
#define TIMER_C_MODE    BIT(14)
#define TIMER_B_MODE    BIT(13)
#define TIMER_A_MODE    BIT(12)

#define TIMER_I_EN      BIT(19)
#define TIMER_H_EN      BIT(18)
#define TIMER_G_EN      BIT(17)
#define TIMER_F_EN      BIT(16)
#define TIMER_I_MODE    BIT(15)
#define TIMER_H_MODE    BIT(14)
#define TIMER_G_MODE    BIT(13)
#define TIMER_F_MODE    BIT(12)

#define TIMER_I_INPUT_CLK 6
#define TIMER_H_INPUT_CLK 4
#define TIMER_G_INPUT_CLK 2
#define TIMER_F_INPUT_CLK 0

#define TIMESTAMP_TIMEBASE_SYSTEM   0b000
#define TIMESTAMP_TIMEBASE_1_US     0b001
#define TIMESTAMP_TIMEBASE_10_US    0b010
#define TIMESTAMP_TIMEBASE_100_US   0b011
#define TIMESTAMP_TIMEBASE_1_MS     0b100

#define TIMEOUT_TIMEBASE_1_US   0b00
#define TIMEOUT_TIMEBASE_10_US  0b01
#define TIMEOUT_TIMEBASE_100_US 0b10
#define TIMEOUT_TIMEBASE_1_MS   0b11

#define TIMER_A_IRQ 42
#define TIMER_B_IRQ 43
#define TIMER_C_IRQ 38
#define TIMER_D_IRQ 61

#define TIMER_F_IRQ 92
#define TIMER_G_IRQ 93
#define TIMER_H_IRQ 94
#define TIMER_I_IRQ 95

typedef struct {
    uint32_t mux;
    uint32_t timer_a;
    uint32_t timer_b;
    uint32_t timer_c;
    uint32_t timer_d;
    uint32_t unused[13];
    uint32_t timer_e;
    uint32_t timer_e_hi;
    uint32_t mux1;
    uint32_t timer_f;
    uint32_t timer_g;
    uint32_t timer_h;
    uint32_t timer_i;
} meson_timer_reg_t;

typedef struct {
    volatile meson_timer_reg_t *regs;
    bool disable;
} meson_timer_t;

typedef struct {
    void *vaddr;
} meson_timer_config_t;

int meson_init(meson_timer_t *timer, meson_timer_config_t config);
uint64_t meson_get_time(meson_timer_t *timer);
void meson_set_timeout(meson_timer_t *timer, uint16_t timeout, bool periodic);
void meson_stop_timer(meson_timer_t *timer);
