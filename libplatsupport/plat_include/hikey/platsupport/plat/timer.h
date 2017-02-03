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
#define DMTIMER1_PADDR DMTIMER0_PADDR
#define DMTIMER2_PADDR 0xF8009000
#define DMTIMER3_PADDR DMTIMER1_PADDR
#define DMTIMER4_PADDR 0xF800A000
#define DMTIMER5_PADDR DMTIMER2_PADDR
#define DMTIMER6_PADDR 0xF800B000
#define DMTIMER7_PADDR DMTIMER3_PADDR
#define DMTIMER8_PADDR 0xF800C000
#define DMTIMER9_PADDR DMTIMER4_PADDR
#define DMTIMER10_PADDR 0xF800D000
#define DMTIMER11_PADDR DMTIMER5_PADDR
#define DMTIMER12_PADDR 0xF800E000
#define DMTIMER13_PADDR DMTIMER6_PADDR
#define DMTIMER14_PADDR 0xF800F000
#define DMTIMER15_PADDR DMTIMER7_PADDR
#define DMTIMER16_PADDR 0xF8010000
#define DMTIMER17_PADDR DMTIMER8_PADDR
#define RTC0_PADDR 0xF8003000
#define RTC1_PADDR 0xF8004000

/* IRQs */
#define DMTIMER0_INTERRUPT 46
#define DMTIMER1_INTERRUPT 47
#define DMTIMER2_INTERRUPT 48
#define DMTIMER3_INTERRUPT 49
#define DMTIMER4_INTERRUPT 50
#define DMTIMER5_INTERRUPT 51
#define DMTIMER6_INTERRUPT 52
#define DMTIMER7_INTERRUPT 53
#define DMTIMER8_INTERRUPT 54
#define DMTIMER9_INTERRUPT 55
#define DMTIMER10_INTERRUPT 56
#define DMTIMER11_INTERRUPT 57
#define DMTIMER12_INTERRUPT 58
#define DMTIMER13_INTERRUPT 59
#define DMTIMER14_INTERRUPT 60
#define DMTIMER15_INTERRUPT 61
#define DMTIMER16_INTERRUPT 62
#define DMTIMER17_INTERRUPT 63
#define RTC0_INTERRUPT 44
#define RTC1_INTERRUPT 40

/* Timers */
enum timer_id {
    DMTIMER0,
    /* The virtual upcounter uses DMTIMER1 internally, so give them
     * the same ID.
     */
    DMTIMER1,
    DMTIMER2,
    DMTIMER3,
    DMTIMER4,
    DMTIMER5,
    DMTIMER6,
    DMTIMER7,
    DMTIMER8,
    DMTIMER9,
    DMTIMER10,
    DMTIMER11,
    DMTIMER12,
    DMTIMER13,
    DMTIMER14,
    DMTIMER15,
    DMTIMER16,
    DMTIMER17,
    RTC0,
    RTC1,
    VIRTUAL_UPCOUNTER,
    NUM_DMTIMERS = 18,
    NUM_RTCS = 2,
    NUM_TIMERS = 21
};

#define TMR_DEFAULT DMTIMER0

static const uintptr_t dm_timer_paddrs[] = {
    [DMTIMER0] = DMTIMER0_PADDR,
    [DMTIMER1] = DMTIMER1_PADDR,
    [DMTIMER2] = DMTIMER2_PADDR,
    [DMTIMER3] = DMTIMER3_PADDR,
    [DMTIMER4] = DMTIMER4_PADDR,
    [DMTIMER5] = DMTIMER5_PADDR,
    [DMTIMER6] = DMTIMER6_PADDR,
    [DMTIMER7] = DMTIMER7_PADDR,
    [DMTIMER8] = DMTIMER8_PADDR,
    [DMTIMER9] = DMTIMER9_PADDR,
    [DMTIMER10] = DMTIMER10_PADDR,
    [DMTIMER11] = DMTIMER11_PADDR,
    [DMTIMER12] = DMTIMER12_PADDR,
    [DMTIMER13] = DMTIMER13_PADDR,
    [DMTIMER14] = DMTIMER14_PADDR,
    [DMTIMER15] = DMTIMER15_PADDR,
    [DMTIMER16] = DMTIMER16_PADDR,
    [DMTIMER17] = DMTIMER17_PADDR,
    [RTC0] = RTC0_PADDR,
    [RTC1] = RTC1_PADDR
};

static const int dm_timer_irqs[] = {
    [DMTIMER0] = DMTIMER0_INTERRUPT,
    [DMTIMER1] = DMTIMER1_INTERRUPT,
    [DMTIMER2] = DMTIMER2_INTERRUPT,
    [DMTIMER3] = DMTIMER3_INTERRUPT,
    [DMTIMER4] = DMTIMER4_INTERRUPT,
    [DMTIMER5] = DMTIMER5_INTERRUPT,
    [DMTIMER6] = DMTIMER6_INTERRUPT,
    [DMTIMER7] = DMTIMER7_INTERRUPT,
    [DMTIMER8] = DMTIMER8_INTERRUPT,
    [DMTIMER9] = DMTIMER9_INTERRUPT,
    [DMTIMER10] = DMTIMER10_INTERRUPT,
    [DMTIMER11] = DMTIMER11_INTERRUPT,
    [DMTIMER12] = DMTIMER12_INTERRUPT,
    [DMTIMER13] = DMTIMER13_INTERRUPT,
    [DMTIMER14] = DMTIMER14_INTERRUPT,
    [DMTIMER15] = DMTIMER15_INTERRUPT,
    [DMTIMER16] = DMTIMER16_INTERRUPT,
    [DMTIMER17] = DMTIMER17_INTERRUPT,
    /* We don't use the RTC's alarm IRQ feature */
    [RTC0] = RTC0_INTERRUPT,
    [RTC1] = RTC1_INTERRUPT
};

/* These two must be populated by the caller of
 * ps_get_timer(), with two pstimer_t* pointers
 * to two pstimer_t handles that were previously
 * returned by a successful call to ps_get_timer().
 *
 * In other words, first call ps_get_timer() on the
 * two timers you plan to use as the back-end for the
 * virtual upcounter, and then finally fill out the
 * two handles you got from those previous two calls,
 * and pass those two (using this struct member) into
 * your call to ps_get_timer(VIRTUAL_UPCOUNTER).
 */
struct pstimer;

typedef struct {
    struct pstimer *rtc_timer, *dualtimer_timer;
} hikey_vupcounter_timer_config_t;

typedef struct {
    void *vaddr;
    uint32_t irq;

    /* When initializing the virtual upcounter, these two
     * IDs state which other two devices you want to use
     * to provide its underlying functionality.
     */
    int rtc_id, dualtimer_id;
    hikey_vupcounter_timer_config_t vupcounter_config;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);

#endif /* _PLATSUPPORT_PLAT_TIMER_H */
