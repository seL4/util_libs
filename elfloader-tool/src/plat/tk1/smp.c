/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>
#include <types.h>

#if CONFIG_MAX_NUM_NODES > 1


void init_cpus(void)
{
    /* SMP booting for Tegra K1 to be implemented */
}

#endif /* CONFIG_MAX_NUM_NODES > 1 */
