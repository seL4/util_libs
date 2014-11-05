/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <errno.h>

#include <platsupport/timer.h>
#include <platsupport/arch/tsc.h>

#include <utils/time.h>

#include <stdbool.h>

#include "../../stubtimer.h"

typedef struct tsc_data {
    uint64_t freq;
} tsc_data_t;


/* interface functions */
static uint64_t
tsc_get_time(const pstimer_t* device)
{
    tsc_data_t *data = (tsc_data_t *) device->data;
    return TSC_TICKS_TO_NS(data->freq);
}
/* static global vars */
static pstimer_t singleton_timer;
static tsc_data_t singleton_data = {0};

/* initialisation function */
pstimer_t *
tsc_get_timer(pstimer_t *timeout_timer)
{
    pstimer_t *tsc_timer = &singleton_timer;

    if (singleton_data.freq == 0) {
        /* timer not initialised yet */
        singleton_data.freq = tsc_calculate_frequency(timeout_timer) / US_IN_S;
        if (singleton_data.freq == 0) {
            /* failed to find freq */
            return NULL;
        }

        stub_timer_get_timer(tsc_timer);
        tsc_timer->get_time = tsc_get_time;
        tsc_timer->data = (void *) &singleton_data;
        tsc_timer->properties.upcounter = true;
        tsc_timer->properties.timeouts = false;
        tsc_timer->properties.bit_width = 64;
        tsc_timer->properties.irqs = 0;
    }

    return tsc_timer;
}

