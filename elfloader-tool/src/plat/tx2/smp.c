/*
 * Copyright 2019, Data61
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
#include <types.h>
#include <elfloader.h>
#include <armv/psci.h>
#include <armv/machine.h>
#include <armv/smp.h>
#include <printf.h>
#include <abort.h>

/* The TX2 has two clusters (Denver2 and A57): Denver2 has two cores,
 * and A57 has 4 cores. The following numbers are used to boot the cores.
 *
 * Denver2: 0 and 1
 * A57: (1 << 8) | [0 | 1 | 2 | 3]. 1 << 8 specifies the cluster id.
 *
 *
 */
#define MAX_CORES           6
#define CLUSTER_SHIFT       8
#define CLUSTER1_NUM_CORES  4

extern void core_entry_head(unsigned long stack);

void init_cpus(void)
{
    int nodes = CONFIG_MAX_NUM_NODES;
    if (nodes > MAX_CORES) {
        printf("CONFIG_MAX_NUM_NODES %d is greater than max number cores %d, will abort\n",
                CONFIG_MAX_NUM_NODES, MAX_CORES);
        abort();
    }
    for (int i = 1; i < nodes; i++) {
        unsigned long target_core = 0;
        if (i <= 3) {
            /* We bring up A57 cores first */
            target_core = (1 << CLUSTER_SHIFT) | i;
        } else {
            /* If we run out of A57 cores, we bring up Denver2 cores */
            target_core = i - CLUSTER1_NUM_CORES;
        }
        int ret = psci_cpu_on(target_core, (unsigned long)core_entry_head, (unsigned long)&core_stacks[i][0]);
        if (ret != PSCI_SUCCESS) {
            printf("Failed to bring up core %d with error %x\n", i, ret);
            abort();
        }
        while (!is_core_up(i));
        printf("Core %d is up with logic ID %d\n", i, i);
    }

    /* set the logic id for the booting core */
    MSR("tpidr_el1", 0);
}

#endif /* CONFIG_MAX_NUM_NODES > 1 */
