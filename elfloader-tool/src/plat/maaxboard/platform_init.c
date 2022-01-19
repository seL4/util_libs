/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, Capgemini Engineering
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <mode/arm_generic_timer.h>

/* Reset the virtual offset for the platform timer to 0 */
void platform_init(void)
{
    reset_cntvoff();
}

#if CONFIG_MAX_NUM_NODES > 1
void non_boot_init(void)
{
    reset_cntvoff();
}
#endif
