/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_TIMER_H
#define _PLATSUPPORT_PLAT_TIMER_H

/* Memory maps */
#define DMTIMER0_PADDR 0xF8008000

/* IRQs */
#define DMTIMER0_INTERRUPT 46

/* Timers */
enum timer_id {
    DMTIMER0,
    NTIMERS
};

#define TMR_DEFAULT DMTIMER0

static const uintptr_t dm_timer_paddrs[] = {
    [DMTIMER0] = DMTIMER0_PADDR,
};

static const int dm_timer_irqs[] = {
    [DMTIMER0] = DMTIMER0_INTERRUPT,
};

typedef struct {
    void *vaddr;
    uint32_t irq;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);

#endif /* _PLATSUPPORT_PLAT_TIMER_H */
