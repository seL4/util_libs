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
/* @AUTHOR(akroh@ertos.nicta.com.au) */
#ifndef _PLATSUPPORT_PLAT_TIMER_H
#define _PLATSUPPORT_PLAT_TIMER_H

#include <platsupport/timer.h>

#include <stdint.h>

/* frequency of the generic timer */
#define PCT_TICKS_PER_US 208llu

/* Memory maps */
#define RPM_TIMERS_PADDR         0x00062000
#define RPM_TIMERS_SIZE          0x1000

#define KPSS_TIMERS_PADDR        0x0200A000
#define KPSS_TIMERS_SIZE         0x1000

#define GSS_TIMERS_PADDR         0x10002000
#define GSS_TIMERS_SIZE          0x1000

#define PPSS_XO_TIMERS_PADDR     0x12080000
#define PPSS_XO_TIMERS_SIZE      0x1000
#define PPSS_SLP_TIMERS_PADDR    0x12081000
#define PPSS_SLP_TIMERS_SIZE     0x1000

/* IRQs */
#define DUMMY_IRQ 99

#define PPSS_XO_TMR0_INTERRUPT  241
#define PPSS_XO_TMR1_INTERRUPT  242
#define PPSS_SLP_TMR0_INTERRUPT 243
#define PPSS_SLP_TMR1_INTERRUPT 244
#define PPSS_SLP_WDOG_INTERRUPT 245

#define KPSS_DGT_INTERRUPT      17
#define KPSS_GPT0_INTERRUPT     18
#define KPSS_GPT1_INTERRUPT     19
#define KPSS_WDT0_INTERRUPT     20
#define KPSS_WDT1_INTERRUPT     21

#define GSS_GPT0_INTERRUPT      DUMMY_IRQ
#define GSS_GPT1_INTERRUPT      DUMMY_IRQ
#define GSS_DGT_INTERRUPT       34 /* ? */
#define GSS_WDT0_INTERRUPT      68 /* ? */
#define GSS_WDT1_INTERRUPT      DUMMY_IRQ

#define RPM_GPT0_INTERRUPT      66
#define RPM_GPT1_INTERRUPT      67
#define RPM_WDOG_INTERRUPT      68

/* Timers */
enum timer_id {
    TMR_PPSS_XO_TMR0,
    TMR_PPSS_XO_TMR1,
    TMR_PPSS_SLP_TMR0,
    TMR_PPSS_SLP_TMR1,
    TMR_PPSS_SLP_WDOG,
    TMR_RPM_GPT0,
    TMR_RPM_GPT1,
    TMR_RPM_WDOG,
    TMR_KPSS_GPT0,
    TMR_KPSS_GPT1,
    TMR_KPSS_DGT,   /* Currently used by the kernel */
    TMR_KPSS_WDT0,
    TMR_KPSS_WDT1,
    TMR_GSS_GPT0,
    TMR_GSS_GPT1,
    TMR_GSS_DGT,
    TMR_GSS_WDT0,
    TMR_GSS_WDT1,
    /* More timers in Audio subsystem 0x28880000 */
    NTIMERS
};
#define TMR_DEFAULT TMR_KPSS_GPT0

static const uintptr_t apq8064_timer_paddrs[] = {
    [TMR_PPSS_XO_TMR0 ] = PPSS_XO_TIMERS_PADDR,
    [TMR_PPSS_XO_TMR1 ] = PPSS_XO_TIMERS_PADDR,
    [TMR_PPSS_SLP_TMR0] = PPSS_SLP_TIMERS_PADDR,
    [TMR_PPSS_SLP_TMR1] = PPSS_SLP_TIMERS_PADDR,
    [TMR_PPSS_SLP_WDOG] = PPSS_SLP_TIMERS_PADDR,
    [TMR_RPM_GPT0     ] = RPM_TIMERS_PADDR,
    [TMR_RPM_GPT1     ] = RPM_TIMERS_PADDR,
    [TMR_RPM_WDOG     ] = RPM_TIMERS_PADDR,
    [TMR_KPSS_GPT0    ] = KPSS_TIMERS_PADDR,
    [TMR_KPSS_GPT1    ] = KPSS_TIMERS_PADDR,
    [TMR_KPSS_DGT     ] = KPSS_TIMERS_PADDR,
    [TMR_KPSS_WDT0    ] = KPSS_TIMERS_PADDR,
    [TMR_KPSS_WDT1    ] = KPSS_TIMERS_PADDR,
    [TMR_GSS_GPT0     ] = GSS_TIMERS_PADDR,
    [TMR_GSS_GPT1     ] = GSS_TIMERS_PADDR,
    [TMR_GSS_DGT      ] = GSS_TIMERS_PADDR,
    [TMR_GSS_WDT0     ] = GSS_TIMERS_PADDR,
    [TMR_GSS_WDT1     ] = GSS_TIMERS_PADDR
};

static const int apq8064_timer_irqs[] = {
    [TMR_PPSS_XO_TMR0 ] = PPSS_XO_TMR0_INTERRUPT,
    [TMR_PPSS_XO_TMR1 ] = PPSS_XO_TMR0_INTERRUPT,
    [TMR_PPSS_SLP_TMR0] = PPSS_SLP_TMR0_INTERRUPT,
    [TMR_PPSS_SLP_TMR1] = PPSS_SLP_TMR1_INTERRUPT,
    [TMR_PPSS_SLP_WDOG] = PPSS_SLP_WDOG_INTERRUPT,
    [TMR_RPM_GPT0     ] = RPM_GPT0_INTERRUPT,
    [TMR_RPM_GPT1     ] = RPM_GPT1_INTERRUPT,
    [TMR_RPM_WDOG     ] = RPM_WDOG_INTERRUPT,
    [TMR_KPSS_GPT0    ] = KPSS_GPT0_INTERRUPT,
    [TMR_KPSS_GPT1    ] = KPSS_GPT1_INTERRUPT,
    [TMR_KPSS_DGT     ] = KPSS_DGT_INTERRUPT,
    [TMR_KPSS_WDT0    ] = KPSS_WDT0_INTERRUPT,
    [TMR_KPSS_WDT1    ] = KPSS_WDT1_INTERRUPT,
    [TMR_GSS_GPT0     ] = GSS_GPT0_INTERRUPT,
    [TMR_GSS_GPT1     ] = GSS_GPT1_INTERRUPT,
    [TMR_GSS_DGT      ] = GSS_DGT_INTERRUPT,
    [TMR_GSS_WDT0     ] = GSS_WDT0_INTERRUPT,
    [TMR_GSS_WDT1     ] = GSS_WDT1_INTERRUPT
};

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);

#endif /* _PLATSUPPORT_PLAT_TIMER_H */
