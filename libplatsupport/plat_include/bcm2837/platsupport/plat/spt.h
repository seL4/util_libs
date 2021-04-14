/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <platsupport/timer.h>

#define SP804_TIMER_IRQ          32

#define BUS_ADDR_OFFSET        0x7E000000
#define PADDDR_OFFSET          0x3F000000

#define SP804_TIMER_BUSADDR    0x7E00B000
#define SP804_TIMER_PADDR      (SP804_TIMER_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)

typedef struct arm_timer {
    uint32_t load;              /* Sets value for timer to count down */
    uint32_t value;             /* Holds the current timer value */
    uint32_t ctrl;              /* Control register for timer */
    uint32_t irq_clear;         /* Clears interrupt pending bit; write only */
    uint32_t raw_irq;           /* Shows status of interrupt pending bit; read only */
    uint32_t masked_irq;        /* Logical and of interrupt pending and interrupt enable */
    uint32_t reload;            /* Also timer reload value, only it doesn't force a reload */
    uint32_t pre_divider;       /* Prescaler, reset value is 0x7D  */
    uint32_t free_run_count;    /* Read only incrementing counter */
} arm_timer_t;

typedef struct {
    freq_t freq;
    volatile arm_timer_t *regs;
    uint32_t prescaler;
    uint64_t counter_start;
} spt_t;

typedef struct {
    /* vaddr timer is mapped to */
    void *vaddr;
} spt_config_t;

UNUSED static timer_properties_t spt_properties = {
    .upcounter = false,
    .timeouts = true,
    .relative_timeouts = true,
    .irqs = 1,
    .bit_width = 32
};

int spt_init(spt_t *spt, spt_config_t config);
uint64_t spt_get_time(spt_t *spt);
int spt_handle_irq(spt_t *spt);
int spt_set_timeout(spt_t *spt, uint64_t ns);
int spt_stop(spt_t *spt);
int spt_start(spt_t *spt);
