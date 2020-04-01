/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <platsupport/timer.h>

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

/* The input frequence is the CPU frequency which is 50MHz by default */
#define APB_TIMER_INPUT_FREQ (50*1000*1000)
#define APB_TIMER_PADDR   0x18000000

/* Multiple timers */
#define APB_TIMER_DIST      0x10
#define APB_TIMER_NUM       2
#define APB_TIMER_BASE(n)   APB_TIMER_DIST * n

#define CMP_WIDTH               32
#define APB_TIMER_CTRL_ENABLE   BIT(0);
#define CMP_MASK                MASK(CMP_WIDTH)

/* Timer IRQs */
#define APB_TIMER_PLIC_BASE     4
#define APB_TIMER_IRQ_OVF(n)    APB_TIMER_PLIC_BASE + 2*n + 0x0
#define APB_TIMER_IRQ_CMP(n)    APB_TIMER_PLIC_BASE + 2*n + 0x1

typedef struct {
    /* vaddr apb_timer is mapped to */
    void *vaddr;
} apb_timer_config_t;

typedef struct apb_timer {
    volatile struct apb_timer_map *apb_timer_map;
    uint64_t time_h;
} apb_timer_t;

struct apb_timer_map {
    uint32_t time;
    uint32_t ctrl;
    uint32_t cmp;
};

int apb_timer_start(apb_timer_t *apb_timer);
int apb_timer_stop(apb_timer_t *apb_timer);
uint64_t apb_timer_get_time(apb_timer_t *apb_timer);
int apb_timer_set_timeout(apb_timer_t *apb_timer, uint64_t ns);
int apb_timer_init(apb_timer_t *apb_timer, apb_timer_config_t config);
