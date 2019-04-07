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

#if CONFIG_MAX_NUM_NODES > 1
#include <types.h>
#include <elfloader.h>
#include <armv/psci.h>
#include <armv/machine.h>
#include <armv/smp.h>
#include <printf.h>
#include <abort.h>

/* The FVP is configurable, so make sure the configuration
 * matches the assumptions below.
 */
#define MAX_CORES               8
#define MAX_CLUSTERS            2
#define CORES_PER_CLUSTER       4
/* The FVP option cluster0/1_mpidr_layout controls
 * if AFF1  or AFFI0 is used as the cpu id.
 * 1 - AFF1, the default value
 * 0 - AFF0
 * Thus, the cluster id is AFF2 when the default
 * value is used.
 */
#define CLUSTER_ID_SHIFT        16

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
        int core_id = i % CORES_PER_CLUSTER;
        int cluster_id = i / CORES_PER_CLUSTER;
        int logic_id = i;
        uint64_t cpuid = 0;
        cpuid = (cluster_id << CLUSTER_ID_SHIFT) | (core_id << 8);
        int ret = psci_cpu_on(cpuid, (unsigned long)core_entry_head, (unsigned long)&core_stacks[logic_id][0]);
        if (ret != PSCI_SUCCESS) {
            printf("Failed to bring up core 0x%lx in cluster %d with error %x with entry %lx\n", cpuid, cluster_id, ret,
                   core_entry_head);
            abort();
        }
        while (!is_core_up(logic_id));
        printf("Core %d in Cluster %d is up with logic ID %d\n", core_id, cluster_id, logic_id);
    }

    /* set the logic id for the booting core */
    MSR("tpidr_el1", 0);
}

#endif
