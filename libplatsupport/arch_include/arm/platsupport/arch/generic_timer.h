/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_GENERIC_TIMER_H
#define __PLAT_SUPPORT_GENERIC_TIMER_H

#include <platsupport/timer.h>
#include <autoconf.h>

#ifdef CONFIG_ARCH_ARM_V7A
#ifdef CONFIG_ARM_CORTEX_A15

pstimer_t *generic_timer_get_timer(void);

#endif /* CONFIG_ARM_CORTEX_A15 */
#endif /* CONFIG_ARCH_ARM_V7A */

#endif /* __PLAT_SUPPORT_PWM_H */
