/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <autoconf.h>
#include <types.h>

#if CONFIG_MAX_NUM_NODES > 1

void init_cpus(void)
{
    /* SMP booting for Tegra X2 to be implemented */
}

#endif /* CONFIG_MAX_NUM_NODES > 1 */
