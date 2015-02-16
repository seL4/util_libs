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
#define DMTIMER2_PADDR 0x48040000
#define DMTIMER3_PADDR 0x48042000
#define DMTIMER4_PADDR 0x48044000
#define DMTIMER5_PADDR 0x48046000
#define DMTIMER6_PADDR 0x48048000
#define DMTIMER7_PADDR 0x4804A000

/* IRQs */
#define DMTIMER2_INTERRUPT 68
#define DMTIMER3_INTERRUPT 69
#define DMTIMER4_INTERRUPT 92
#define DMTIMER5_INTERRUPT 93
#define DMTIMER6_INTERRUPT 94
#define DMTIMER7_INTERRUPT 95

/* Timers */
enum timer_id {
    DMTIMER2,
    DMTIMER3,
    DMTIMER4,
    DMTIMER5,
    DMTIMER6,
    DMTIMER7,
    NTIMERS
};
#define TMR_DEFAULT DMTIMER2

static const uintptr_t dm_timer_paddrs[] = {
    [DMTIMER2] = DMTIMER2_PADDR,
    [DMTIMER3] = DMTIMER3_PADDR,
    [DMTIMER4] = DMTIMER4_PADDR,
    [DMTIMER5] = DMTIMER5_PADDR,
    [DMTIMER6] = DMTIMER6_PADDR,
    [DMTIMER7] = DMTIMER7_PADDR,
};

static const int dm_timer_irqs[] = {
    [DMTIMER2] = DMTIMER2_INTERRUPT,
    [DMTIMER3] = DMTIMER3_INTERRUPT,
    [DMTIMER4] = DMTIMER4_INTERRUPT,
    [DMTIMER5] = DMTIMER5_INTERRUPT,
    [DMTIMER6] = DMTIMER6_INTERRUPT,
    [DMTIMER7] = DMTIMER7_INTERRUPT,
};

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
    uint32_t irq;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);

#endif /* _PLATSUPPORT_PLAT_TIMER_H */
