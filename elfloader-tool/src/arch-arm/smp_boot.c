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

#include <printf.h>
#include <cpuid.h>
#include <abort.h>

#include <elfloader.h>

#if CONFIG_MAX_NUM_NODES > 1
VISIBLE volatile word_t smp_aps_index = 1;
static volatile int non_boot_lock = 0;

/* Entry point for all CPUs other than the initial. */
void non_boot_main(void)
{
    /* Spin until the first CPU has finished initialisation. */
    while (!non_boot_lock) {
        cpu_idle();
    }

    /* Enable the MMU, and enter the kernel. */
    if(is_hyp_mode()){
        arm_enable_hyp_mmu();
    }else{
        arm_enable_mmu();
    }

    /* Jump to the kernel. */
    ((init_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                            user_info.phys_region_end, user_info.phys_virt_offset,
                                            user_info.virt_entry);

    printf("AP Kernel returned back to the elf-loader.\n");
    abort();
}

void smp_boot(void)
{
    init_cpus();
    non_boot_lock = 1;
}
#endif /* CONFIG_MAX_NUM_NODES */
