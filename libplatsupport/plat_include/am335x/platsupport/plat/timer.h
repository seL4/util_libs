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
typedef enum timer_id {
    DMTIMER2,
    DMTIMER3,
    DMTIMER4,
    DMTIMER5,
    DMTIMER6,
    DMTIMER7,
    NTIMERS
} dmt_id_t;
#define TMR_DEFAULT DMTIMER2

static const uintptr_t dmt_paddrs[] = {
    [DMTIMER2] = DMTIMER2_PADDR,
    [DMTIMER3] = DMTIMER3_PADDR,
    [DMTIMER4] = DMTIMER4_PADDR,
    [DMTIMER5] = DMTIMER5_PADDR,
    [DMTIMER6] = DMTIMER6_PADDR,
    [DMTIMER7] = DMTIMER7_PADDR,
};

static const int dmt_irqs[] = {
    [DMTIMER2] = DMTIMER2_INTERRUPT,
    [DMTIMER3] = DMTIMER3_INTERRUPT,
    [DMTIMER4] = DMTIMER4_INTERRUPT,
    [DMTIMER5] = DMTIMER5_INTERRUPT,
    [DMTIMER6] = DMTIMER6_INTERRUPT,
    [DMTIMER7] = DMTIMER7_INTERRUPT,
};

static UNUSED timer_properties_t dmt_properties = {
    .upcounter = false,
    .timeouts = true,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
    dmt_id_t id;
} dmt_config_t;

struct dmt_map {
    uint32_t tidr; // 00h TIDR Identification Register
    uint32_t padding1[3];
    uint32_t cfg; // 10h TIOCP_CFG Timer OCP Configuration Register
    uint32_t padding2[3];
    uint32_t tieoi; // 20h IRQ_EOI Timer IRQ End-Of-Interrupt Register
    uint32_t tisrr; // 24h IRQSTATUS_RAW Timer IRQSTATUS Raw Register
    uint32_t tisr; // 28h IRQSTATUS Timer IRQSTATUS Register
    uint32_t tier; // 2Ch IRQENABLE_SET Timer IRQENABLE Set Register
    uint32_t ticr; // 30h IRQENABLE_CLR Timer IRQENABLE Clear Register
    uint32_t twer; // 34h IRQWAKEEN Timer IRQ Wakeup Enable Register
    uint32_t tclr; // 38h TCLR Timer Control Register
    uint32_t tcrr; // 3Ch TCRR Timer Counter Register
    uint32_t tldr; // 40h TLDR Timer Load Register
    uint32_t ttgr; // 44h TTGR Timer Trigger Register
    uint32_t twps; // 48h TWPS Timer Write Posted Status Register
    uint32_t tmar; // 4Ch TMAR Timer Match Register
    uint32_t tcar1; // 50h TCAR1 Timer Capture Register
    uint32_t tsicr; // 54h TSICR Timer Synchronous Interface Control Register
    uint32_t tcar2; // 58h TCAR2 Timer Capture Register
};

typedef struct dmt {
    volatile struct dmt_map *hw;
} dmt_t;

static inline void *dmt_paddr(dmt_id_t id) {
    if (id <= DMTIMER7 && id >= DMTIMER2) {
        return  (void *) dmt_paddrs[id];
    } else {
        return NULL;
    }
}

static inline long dmt_irq(dmt_id_t id) {
    if (id <= DMTIMER7 && id >= DMTIMER2) {
        return dmt_irqs[id];
    } else {
        return 0;
    }
}

int dmt_init(dmt_t *dmt, dmt_config_t config);
int dmt_start(dmt_t *dmt);
int dmt_stop(dmt_t *dmt);
int dmt_set_timeout(dmt_t *dmt, uint64_t ns, bool periodic);
void dmt_handle_irq(dmt_t *dmt);
