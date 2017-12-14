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

#include <platsupport/plat/epit_constants.h>
#include <platsupport/timer.h>

#include <stdint.h>

typedef struct {
    /* vaddr epit is mapped to */
    void *vaddr;
    /* irq for this epit (should be EPIT_INTERRUPT1 or EPIT_INTERRUPT2 depending on the epit */
    uint32_t irq;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} epit_config_t;

struct epit_map;
typedef struct epit {
    volatile struct epit_map *epit_map;
    uint32_t prescaler;
} epit_t;

static inline timer_properties_t
epit_timer_properties(void)
{
   return (timer_properties_t) {
     .upcounter = false,
     .timeouts = true,
     .relative_timeouts = true,
     .absolute_timeouts = false,
     .periodic_timeouts = true,
     .bit_width = 32,
     .irqs = 1,
   };
}

/* initialise an epit struct */
int epit_init(epit_t *epit, epit_config_t config);
/* turn off any pending irqs */
int epit_stop(epit_t *epit);
/* set a relative timeout */
int epit_set_timeout(epit_t *epit, uint64_t ns, bool periodic);
/* set a relative timeout in ticks */
int epit_set_timeout_ticks(epit_t *epit, uint64_t ticks, bool periodic);
/* handle an irq */
int epit_handle_irq(epit_t *epit);
