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
#pragma once

#include <platsupport/timer.h>

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

/* Timers */
typedef enum timer_id {
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
    NUM_DMTIMERS = 18,
} dmt_id_t;

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
};

static const long dm_timer_irqs[] = {
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
};

static inline void *dmt_get_paddr(dmt_id_t id)
{
    if (id < NUM_DMTIMERS && id >= DMTIMER0) {
        return (void *) dm_timer_paddrs[id];
    }
    return NULL;
}

static inline long dmt_get_irq(dmt_id_t id)
{
    if (id < NUM_DMTIMERS && id >= DMTIMER0) {
        return dm_timer_irqs[id];
    }
    return 0;
}

static UNUSED timer_properties_t dmtimer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    void *regs;
} dmt_t;

typedef struct {
    void *vaddr;
    int id;
} dmt_config_t;

int dmt_init(dmt_t *dmt, dmt_config_t config);
void dmt_handle_irq(dmt_t *dmt);
/* convert between dmt ticks and ns */
uint64_t dmt_ticks_to_ns(uint64_t ticks);
/* return true if an overflow irq is pending */
bool dmt_is_irq_pending(dmt_t *dmt);
int dmt_set_timeout_ticks(dmt_t *dmt, uint32_t ticks, bool periodic, bool irqs);
/* set a timeout in nano seconds */
int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic, bool irqs);
int dmt_start(dmt_t *dmt);
int dmt_stop(dmt_t *dmt);
uint64_t dmt_get_time(dmt_t *dmt);
uint64_t dmt_get_ticks(dmt_t *dmt);
