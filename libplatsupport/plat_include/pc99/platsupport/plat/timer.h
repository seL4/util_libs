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
#include <platsupport/ltimer.h>
#include <platsupport/plat/pit.h>
#include <platsupport/plat/hpet.h>


/* Using the default function, the pc99 ltimer will try to use the HPET and then fall back to the PIT,
 * using the TSC for timestamps
 *
 * This file provides functinos for specifically picking your ltimer implementation
 */

/* Functions for specifically setting up a HPET based ltimer */

/* Initialise description functions for a HPET backed ltimer, providing the HPET region but
 * using the default irq - populate the irq struct with details  */
int ltimer_hpet_describe_with_region(ltimer_t *ltimer, ps_io_ops_t ops, pmem_region_t region, ps_irq_t *irq);
/* Initialise the description functions for a HPET based ltimer using the provided irq and region */
int ltimer_hpet_describe(ltimer_t *ltimer, ps_io_ops_t ops, ps_irq_t irq, pmem_region_t hpet_region);
/* Initialise a hpet based ltimer using the provided irq and region */
int ltimer_hpet_init(ltimer_t *ltimer, ps_io_ops_t ops, ps_irq_t irq, pmem_region_t hpet_region);

/* Functions for specifically setting up a PIT based ltimer */
int ltimer_pit_describe(ltimer_t *ltimer, ps_io_ops_t ops);
int ltimer_pit_init(ltimer_t *ltimer, ps_io_ops_t ops);
int ltimer_pit_init_freq(ltimer_t *ltimer, ps_io_ops_t ops, uint64_t tsc_freq);
