/*
 * Copyright 2017, Data61
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

#define CPU_JUMP_PTR              0xFFFFFFF0

#include <printf.h>
#include <types.h>
#include <scu.h>
#include <abort.h>

extern void non_boot_core(void);

static void *get_scu_base(void)
{
    void *scu;
    asm("mrc p15, 4, %0, c15, c0, 0" : "=r" (scu));
    return scu;
}


static void
boot_cpus(void (*entry)(void))
{
    *((volatile uint32_t*)CPU_JUMP_PTR) = (uint32_t)entry;
    asm volatile ("dsb;");
    asm volatile ("sev;");
}

void
init_cpus(void)
{
    unsigned int num;
    void *scu = get_scu_base();

    num = scu_get_core_count(scu);
    /* Currently there is no support for choosing which CPUs to boot, however,
     * there are only 2 CPUs on the Zynq7000. Either we boot no additional CPUs
     * or we boot all additional CPUs.
     */

    if (num > CONFIG_MAX_NUM_NODES) {
        num = CONFIG_MAX_NUM_NODES;
    } else if (num < CONFIG_MAX_NUM_NODES) {
        printf("Error: Unsupported number of CPUs! This platform has %u CPUs, while static configuration provided is %u CPUs\n", num, CONFIG_MAX_NUM_NODES);
        abort();
    }

    printf("Bringing up %d other cpus\n", num - 1);
    if(num != 1){
        boot_cpus(&non_boot_core);
    }
}

#endif /* CONFIG_MAX_NUM_NODES > 1 */
