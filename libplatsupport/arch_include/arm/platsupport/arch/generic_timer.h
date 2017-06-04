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
