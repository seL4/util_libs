/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/driver/sp804/sp804_ltimer.h>

#include "../../ltimer.h"

/* FVP has two sp804 timers, each running at 35 MHz
 *  /soc/timer@1c110000
 *  /soc/timer@1c120000
 */
#define SP804_LTIMER_PATH   "/soc/timer@1c110000"
#define SP804_LTIMER_FREQ   35000000

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops,
                        ltimer_callback_fn_t callback, void *callback_token)
{
    return ltimer_sp804_init(ltimer, SP804_LTIMER_PATH, SP804_LTIMER_FREQ, ops,
                             callback, callback_token);
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
