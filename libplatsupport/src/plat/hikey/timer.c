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

#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>

#include "timer_priv.h"

pstimer_t *
ps_get_timer(enum timer_id id, timer_config_t *config)
{
    if (id > NUM_TIMERS) {
        return NULL;
    }

    if (id <= DMTIMER17) {
        return hikey_dualtimer_get_timer(id, config);
    } else if (id <= RTC1) {
        return hikey_rtc_get_timer(id, config);
    } else {
        return hikey_vupcounter_get_timer(config->rtc_id, config->dualtimer_id,
                                          &config->vupcounter_config);
    }
}
