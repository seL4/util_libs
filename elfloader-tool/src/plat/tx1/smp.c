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

#if CONFIG_MAX_NUM_NODES > 1
#include <types.h>
#include <elfloader.h>
#include <armv/psci.h>
#include <armv/machine.h>
#include <armv/smp.h>
#include <printf.h>
#include <abort.h>


/* The TX1 has two clusters (A57 and A53), and each cluster has 4
 * cores. But "the CPU subsystem supports a switch-cluster mode meaning
 * that only one of the clusters can be active at any given time."
 * Thus, we do not bother to bring the A53 cluster at all.
 */
#define MAX_CORES           4

extern void core_entry_head(void);

/* It seems that the last parameter of the psci_cpu_on call is not passed correctly
 * to the x0 register of the core that we want to activate, so we have to give the
 * stack address allocated to the core in memory.
 */

void init_cpus(void)
{
    int nodes = CONFIG_MAX_NUM_NODES;
    if (nodes > MAX_CORES) {
        printf("CONFIG_MAX_NUM_NODES %d is greater than max number cores %d, will abort\n",
               CONFIG_MAX_NUM_NODES, MAX_CORES);
        abort();
    }
    for (int i = 1; i < nodes; i++) {
        /* all cores read the stack pointer from the same place */
        core_stacks[0][0] = (unsigned long) &core_stacks[i][0];
        asm volatile("dsb sy":::"memory");
        int ret = psci_cpu_on(i, (unsigned long)core_entry_head, 0);
        if (ret != PSCI_SUCCESS) {
            printf("Failed to bring up core %d with error %d\n", i, ret);
            abort();
        }
        while (!is_core_up(i));
        printf("Core %d is up with logic ID %d\n", i, i);
    }

    /* set the logic id for the booting core */
    MSR("tpidr_el1", 0);
}

#endif
