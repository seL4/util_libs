/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_PWM_H
#define __PLAT_SUPPORT_PWM_H

#include <platsupport/timer.h>
#include <platsupport/plat/pwm.h>

#include <stdint.h>

typedef struct {
    /* vaddr pwm is mapped to */
    void *vaddr;
} pwm_config_t;

pstimer_t *pwm_get_timer(pwm_config_t *config);

#endif /* __PLAT_SUPPORT_PWM_H */
