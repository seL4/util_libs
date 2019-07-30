/*
 * Copyright 2019, DornerWorks
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_GPL)
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>

#if CONFIG_MAX_NUM_NODES > 1
#include <types.h>
#include <elfloader.h>
#include <armv/psci.h>
#include <armv/machine.h>
#include <armv/smp.h>
#include <printf.h>
#include <abort.h>

#define MAX_CORES               4

extern void core_entry_head(void);

void init_cpus(void)
{
    int nodes = CONFIG_MAX_NUM_NODES;

    if (nodes > MAX_CORES) {
        printf("CONFIG_MAX_NUM_NODES %d is greater than max number cores %d, will abort\n",
               CONFIG_MAX_NUM_NODES, MAX_CORES);
        abort();
    }

    for (int i = 1; i < nodes; i++) {
        int ret = psci_cpu_on(i, (unsigned long)core_entry_head, (unsigned long)&core_stacks[i][0]);
        if (ret != PSCI_SUCCESS) {
            printf("Failed to bring up core 0x%lx in with error %x with entry %lx\n", i, ret,
                   core_entry_head);
            abort();
        }
        while (!is_core_up(i));
        printf("Core %d is up with logic ID %d\n", i, i);
    }

    /* set the logic id for the booting core */
    MSR("tpidr_el1", 0);
}
#endif
