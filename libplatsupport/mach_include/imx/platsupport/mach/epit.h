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

#include <platsupport/io.h>
#include <platsupport/ltimer.h>
#include <platsupport/fdt.h>
#include <platsupport/timer.h>
#include <platsupport/plat/epit_constants.h>

#include <stdint.h>

typedef struct {
    /* initialised ps_io_ops_t structure to allocate resources with */
    ps_io_ops_t io_ops;
    /* user callback function to be called on interrupt */
    ltimer_callback_fn_t user_callback;
    /* token to be passed into the callback function */
    void *user_callback_token;
    /* path to the epit node in the DTB */
    char *device_path;
    /* flag determining if this timer should be configured as a timestamp timer */
    bool is_timestamp;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} epit_config_t;

struct epit_map;
typedef struct epit {
    ps_io_ops_t io_ops;
    irq_id_t irq_id;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    pmem_region_t timer_pmem;
    volatile struct epit_map *epit_map;
    uint32_t prescaler;
    bool is_timestamp;
    uint64_t high_bits;
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
/* destroy the epit */
int epit_destroy(epit_t *epit);
/* turn off any pending irqs */
int epit_stop(epit_t *epit);
/* set a relative timeout */
int epit_set_timeout(epit_t *epit, uint64_t ns, bool periodic);
/* set a relative timeout in ticks */
int epit_set_timeout_ticks(epit_t *epit, uint64_t ticks, bool periodic);
/* convert epit ticks to ns */
uint64_t epit_get_time(epit_t *epit);
