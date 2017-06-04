/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_TIMER_H
#define _PLATSUPPORT_PLAT_TIMER_H

#include <platsupport/clock.h>

/* Memory maps */
#define TTC0_PADDR               0xF8001000
#define TTC1_PADDR               0xF8002000

#define TTC_TIMER_SIZE           0x1000
#define TTC0_TIMER_SIZE          TTC_TIMER_SIZE
#define TTC1_TIMER_SIZE          TTC_TIMER_SIZE

/* IRQs */
#define TTC0_TIMER1_IRQ          42
#define TTC0_TIMER2_IRQ          43
#define TTC0_TIMER3_IRQ          44
#define TTC1_TIMER1_IRQ          69
#define TTC1_TIMER2_IRQ          70
#define TTC1_TIMER3_IRQ          71

/* Timers */
enum timer_id {
    TTC0_TIMER1,
    TTC0_TIMER2,
    TTC0_TIMER3,
    TTC1_TIMER1,
    TTC1_TIMER2,
    TTC1_TIMER3,
    NTIMERS
};
#define TMR_DEFAULT TTC0_TIMER1

static const uintptr_t zynq_timer_paddrs[] = {
    [TTC0_TIMER1] = TTC0_PADDR,
    [TTC0_TIMER2] = TTC0_PADDR,
    [TTC0_TIMER3] = TTC0_PADDR,
    [TTC1_TIMER1] = TTC1_PADDR,
    [TTC1_TIMER2] = TTC1_PADDR,
    [TTC1_TIMER3] = TTC1_PADDR
};

static const int zynq_timer_irqs[] = {
    [TTC0_TIMER1] = TTC0_TIMER1_IRQ,
    [TTC0_TIMER2] = TTC0_TIMER2_IRQ,
    [TTC0_TIMER3] = TTC0_TIMER3_IRQ,
    [TTC1_TIMER1] = TTC1_TIMER1_IRQ,
    [TTC1_TIMER2] = TTC1_TIMER2_IRQ,
    [TTC1_TIMER3] = TTC1_TIMER3_IRQ
};

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
    clk_t* clk_src;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);

#endif /* _PLATSUPPORT_PLAT_TIMER_H */
