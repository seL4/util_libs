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

#define SP804_TIMER_IRQ          32

#define BUS_ADDR_OFFSET        0x7E000000
#define PADDDR_OFFSET          0x3F000000

#define SP804_TIMER_BUSADDR    0x7E00B000
#define SP804_TIMER_PADDR      (SP804_TIMER_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)

/* Timers */
enum timer_id {
    SP804,
    NTIMERS
};
#define TMR_DEFAULT SP804

static const uintptr_t bcm_timer_paddrs[] = {
    [SP804] = SP804_TIMER_PADDR,

};

static const int bcm_timer_irqs[] = {
    [SP804] = SP804_TIMER_IRQ,
};

typedef struct {
    /* vaddr timer is mapped to */
    void *vaddr;
} timer_config_t;

pstimer_t *ps_get_timer(enum timer_id id, timer_config_t *config);
