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

#ifdef CONFIG_SMP_ARM_MPCORE

#define CPU_JUMP_PTR              0xFFFFFFF0

#include <printf.h>
#include <types.h>
#include <scu.h>

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
#ifdef CONFIG_MAX_NUM_NODES
    /* Currently there is no support for choosing which CPUs to boot, however,
     * there are only 2 CPUs on the Zynq7000. Either we boot no additional CPUs
     * or we boot all additional CPUs.
     */

    if (num > CONFIG_MAX_NUM_NODES) {
        num = CONFIG_MAX_NUM_NODES;
    }
#endif
    printf("Bringing up %d other cpus\n", num - 1);
    if(num != 1){
        boot_cpus(&non_boot_core);
    }
}

#endif
