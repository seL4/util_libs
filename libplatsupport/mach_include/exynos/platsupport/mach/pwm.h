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
#include <platsupport/plat/pwm.h>

#include <stdint.h>

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
} pwm_config_t;

pstimer_t *pwm_get_timer(pwm_config_t *config);

