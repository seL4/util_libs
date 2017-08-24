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
#include <platsupport/plat/gpt_constants.h>

#include <stdint.h>

typedef struct {
    /* vaddr gpt is mapped to */
    void *vaddr;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} gpt_config_t;

struct gpt_map;

typedef struct gpt {
    volatile struct gpt_map *gpt_map;
    uint32_t prescaler;
    uint32_t high_bits;
} gpt_t;

/* More can be done with this timer
 * but this driver can only count up
 * currently.
 */
static inline timer_properties_t
gpt_get_properies(void) {
    return (timer_properties_t) {
		.upcounter = true,
        .timeouts = true,
        .absolute_timeouts = false,
        .relative_timeouts = true,
        .periodic_timeouts = true,
        .bit_width = 32,
        .irqs = 1,
	};
}

/*
 * Initialise a passed in gpt struct with the provided config
 */
int gpt_init(gpt_t *gpt, gpt_config_t config);

/* start the gpt */
int gpt_start(gpt_t *gpt);
/* stop the gpt */
int gpt_stop(gpt_t  *gpt);
/* handle an irq */
void gpt_handle_irq(gpt_t *gpt);
/* read the value of the current time in ns */
uint64_t gpt_get_time(gpt_t *gpt);
/* set a relative timeout */
/* WARNING: Please note that once a GPT is set to trigger timeout interrupt(s),
 * it can not be used for the timekeeping purpose (i.e. the gpt_get_time
 * will not work on the GPT). The GPT has to be reinitialised by calling
 * gpt_init again in order to be used a timekeeping clock.
 */
int gpt_set_timeout(gpt_t *gpt, uint64_t ns, bool periodic);

