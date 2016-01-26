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
#include "efi/efi.h"

#include "elfloader.h"
#include "cpuid.h"
#include <platform.h>

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

#ifdef CONFIG_IMAGE_EFI
    if (efi_exit_boot_services() != EFI_SUCCESS) {
        printf("Unable to exit UEFI boot services!\n");
        abort();
    }
#endif

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
    if(is_hyp_mode()){
        printf("Enabling hypervisor MMU and paging\n");
        init_hyp_boot_vspace(&kernel_info);
        arm_enable_hyp_mmu();
    }

    /* If we are in HYP mode, we enable the SV MMU and paging
     * just in case the kernel does not support hyp mode. */
    printf("Enabling MMU and paging\n");
    init_boot_vspace(&kernel_info);
    arm_enable_mmu();

#ifdef CONFIG_SMP_ARM_MPCORE
    /* Bring up any other CPUs */
    init_cpus();
    non_boot_lock = 1;
#endif

    /* Enter kernel. */
    if (UART_PPTR < kernel_info.virt_region_start) {
        printf("Jumping to kernel-image entry point...\n\n");
    } else {
        /* Our serial port is no longer accessible */
    }

    ((init_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                            user_info.phys_region_end, user_info.phys_virt_offset,
                                            user_info.virt_entry);

    /* We should never get here. */
    printf("Kernel returned back to the elf-loader.\n");
    abort();
}
