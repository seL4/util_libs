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
#include <autoconf.h>


/*
 * This timer will only work if the kernel has configured
 * CNTPCT to be read from user-level. This is done by writing 1 to CNTKCTL.
 * If CONFIG_DANGEROUS_CODE_INJECTION is available, we'll try that,
 * otherwise we will use a default frequency.
 *
 * If all else fails the timer will fail to initialise.
 *
 */
typedef struct {
    uint32_t freq;
} generic_timer_t;

static inline timer_properties_t
get_generic_timer_properties(void) {
    return (timer_properties_t) {
        .upcounter = true,
        .bit_width = 64
    };
}

int generic_timer_init(generic_timer_t *timer);
uint64_t generic_timer_get_time(generic_timer_t *timer);
