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
#include <autoconf.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/arch/generic_timer.h>

uint64_t generic_timer_get_time(generic_timer_t *timer)
{
    return freq_cycles_and_hz_to_ns(generic_timer_get_ticks(), timer->freq);
}

int generic_timer_get_init(generic_timer_t *timer)
{

    if (timer == NULL) {
        ZF_LOGE("Must provide memory for generic timer");
        return EINVAL;
    }

    if (!config_set(CONFIG_EXPORT_PCNT_USER)) {
        ZF_LOGE("Generic timer not exported!");
        return ENXIO;
    }

    /* try to read the frequency */
    timer->freq = generic_timer_get_freq();

#ifdef PCT_TICKS_PER_US
    if (timer->freq == 0) {
        freq = PCT_TICKS_PER_US;
    }
#endif

    if (timer->freq == 0) {
        /* fail init, we don't know what the frequency of the timer is */
        ZF_LOGE("Failed to find generic timer frequency");
        return ENXIO;
    }

    return 0;
}
