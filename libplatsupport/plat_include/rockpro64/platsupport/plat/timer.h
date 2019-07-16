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

/* Memory maps */
#define RKTIMER0_PADDR 0xFF850000
#define RKTIMER1_PADDR 0xFF850020
#define RKTIMER2_PADDR 0xFF850040
#define RKTIMER3_PADDR 0xFF850060
#define RKTIMER4_PADDR 0xFF850080
#define RKTIMER5_PADDR 0xFF8500A0
#define RKTIMER6_PADDR 0xFF858000

/* IRQs */
#define RKTIMER0_INTERRUPT 113
#define RKTIMER1_INTERRUPT 114
#define RKTIMER2_INTERRUPT 115
#define RKTIMER3_INTERRUPT 116
#define RKTIMER4_INTERRUPT 117
#define RKTIMER5_INTERRUPT 118
#define RKTIMER6_INTERRUPT 119

/* Timers */
typedef enum timer_id {
    RKTIMER0 = 0,
    RKTIMER1,
    RKTIMER2,
    RKTIMER3,
    RKTIMER4,
    RKTIMER5,
    NTIMERS
} rk_id_t;

#define TMR_DEFAULT RKTIMER0

static const uintptr_t rk_paddrs[] = {
    [RKTIMER0] = RKTIMER0_PADDR,
    [RKTIMER1] = RKTIMER1_PADDR,
    [RKTIMER2] = RKTIMER2_PADDR,
    [RKTIMER3] = RKTIMER3_PADDR,
    [RKTIMER4] = RKTIMER4_PADDR,
    [RKTIMER5] = RKTIMER5_PADDR,
};

static const int rk_irqs[] = {
    [RKTIMER0] = RKTIMER0_INTERRUPT,
    [RKTIMER1] = RKTIMER1_INTERRUPT,
    [RKTIMER2] = RKTIMER2_INTERRUPT,
    [RKTIMER3] = RKTIMER3_INTERRUPT,
    [RKTIMER4] = RKTIMER4_INTERRUPT,
    [RKTIMER5] = RKTIMER5_INTERRUPT,
};

static UNUSED timer_properties_t rk_properties = {
    .upcounter = true,
    .timeouts = true,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    void *vaddr;
    rk_id_t id;
} rk_config_t;

struct rk_map {
    uint32_t load_count0;
    uint32_t load_count1;
    uint32_t current_value0;
    uint32_t current_value1;
    uint32_t load_count2;
    uint32_t load_count3;
    uint32_t interrupt_status;
    uint32_t control_register;
};

typedef struct rk {
    volatile struct rk_map *hw;
} rk_t;

static inline void *rk_paddr(rk_id_t id) {
    if (id <= RKTIMER5 && id >= RKTIMER0) {
        return  (void *) rk_paddrs[id];
    } else {
        return NULL;
    }
}

static inline long rk_irq(rk_id_t id) {
    if (id <= RKTIMER5 && id >= RKTIMER0) {
        return rk_irqs[id];
    } else {
        return 0;
    }
}

enum ttype {
    TIMER_RK = 0,
    TIMEOUT_RK
};

int rk_init(rk_t *rk, rk_config_t config);
int rk_start(rk_t *rk, enum ttype type);
int rk_stop(rk_t *rk);
uint64_t rk_get_time(rk_t *rk);
int rk_set_timeout(rk_t *rk, uint64_t ns, bool periodic);
void rk_handle_irq(rk_t *rk);
/* return true if a match is pending */
bool rk_pending_match(rk_t *rk);
