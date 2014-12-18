/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "elfloader.h"
#include "cpuid.h"

static struct image_info kernel_info;
static struct image_info user_info;

typedef void (*init_kernel_t)(paddr_t ui_p_reg_start,
                              paddr_t ui_p_reg_end, int32_t pv_offset, vaddr_t v_entry);

/* Poor-man's lock. */
static volatile int non_boot_lock = 0;

#ifdef CONFIG_SMP_ARM_MPCORE
/* External symbols. */
extern uint32_t booting_cpu_id;
#endif

/* Entry point for all CPUs other than the initial. */
void non_boot_main(void)
{
#ifdef CONFIG_SMP_ARM_MPCORE
    int cpu_mode;
    /* Spin until the first CPU has finished intialisation. */
    while (!non_boot_lock) {
        cpu_idle();
    }

    /* Enable the MMU, and enter the kernel. */
    cpu_mode = read_cpsr() & CPSR_MODE_MASK;
    if(cpu_mode == CPSR_MODE_HYPERVISOR){
        arm_enable_hyp_mmu();
    }else{
        arm_enable_mmu();
    }

    /* Jump to the kernel. */
    ((init_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                            user_info.phys_region_end, user_info.phys_virt_offset,
                                            user_info.virt_entry);
#endif
}

/*
 * Entry point.
 *
 * Unpack images, setup the MMU, jump to the kernel.
 */
void main(void)
{
    int num_apps;
    int cpu_mode;

#ifdef CONFIG_SMP_ARM_MPCORE
    /* If not the boot strap processor then go to non boot main */
    if ( (read_cpuid_mpidr() & 0xf) != booting_cpu_id) {
        non_boot_main();
    }
#endif

    /* Print welcome message. */
    printf("\nELF-loader started on ");
    print_cpuid();
    platform_init();

    printf("  paddr=[%p..%p]\n", _start, _end - 1);

    /* Unpack ELF images into memory. */
    load_images(&kernel_info, &user_info, 1, &num_apps);
    if (num_apps != 1) {
        printf("No user images loaded!\n");
        abort();
    }

    /* Setup MMU. */
    cpu_mode = read_cpsr() & CPSR_MODE_MASK;
    if(cpu_mode == CPSR_MODE_HYPERVISOR){
        printf("Enabling hypervisor MMU and paging\n");
        init_lpae_boot_pd(&kernel_info);
        arm_enable_hyp_mmu();
    }
    /* If we are in HYP mode, we enable the SV MMU and paging
     * just in case the kernel does not support hyp mode. */
    printf("Enabling MMU and paging\n");
    init_boot_pd(&kernel_info);
    arm_enable_mmu();

#ifdef CONFIG_SMP_ARM_MPCORE
    /* Bring up any other CPUs */
    init_cpus();
    non_boot_lock = 1;
#endif

    /* Enter kernel. */
#ifdef PLAT_ZYNQ7000
    /* Our serial port is no longer accessible */
#else
    printf("Jumping to kernel-image entry point...\n\n");
#endif
    ((init_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                            user_info.phys_region_end, user_info.phys_virt_offset,
                                            user_info.virt_entry);

    /* We should never get here. */
    printf("Kernel returned back to the elf-loader.\n");
    abort();
}
