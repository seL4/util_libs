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

#include <printf.h>
#include <types.h>
#include <abort.h>
#include <strops.h>
#include <cpuid.h>
#include <platform.h>

#include "efi/efi.h"
#include <elfloader.h>

ALIGN(BIT(PAGE_BITS)) VISIBLE
char core_stack_alloc[CONFIG_MAX_NUM_NODES][BIT(PAGE_BITS)];
VISIBLE volatile word_t smp_aps_index = 1;
static volatile int non_boot_lock = 0;

static struct image_info kernel_info;
static struct image_info user_info;

typedef void (*init_kernel_t)(paddr_t ui_p_reg_start,
                              paddr_t ui_p_reg_end, int32_t pv_offset, vaddr_t v_entry);

#if CONFIG_MAX_NUM_NODES > 1
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
#endif /* CONFIG_MAX_NUM_NODES */

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

    /* Print welcome message. */
    platform_init();
    printf("\nELF-loader started on ");
    print_cpuid();

    printf("  paddr=[%p..%p]\n", _start, _end - 1);

    /* Unpack ELF images into memory. */
    load_images(&kernel_info, &user_info, 1, &num_apps);
    if (num_apps != 1) {
        printf("No user images loaded!\n");
        abort();
    }

#if (defined(CONFIG_ARCH_ARM_V7A) || defined(CONFIG_ARCH_ARM_V8A)) && !defined(CONFIG_ARM_HYPERVISOR_SUPPORT)
    if(is_hyp_mode()){
    extern void leave_hyp(void);
        leave_hyp();
    }
#endif
    /* Setup MMU. */
    if(is_hyp_mode()){
        init_hyp_boot_vspace(&kernel_info);
    }
    /* If we are in HYP mode, we enable the SV MMU and paging
     * just in case the kernel does not support hyp mode. */
    init_boot_vspace(&kernel_info);

#if CONFIG_MAX_NUM_NODES > 1
    /* Bring up any other CPUs before switching PD in case
     * required device memory exists in the kernel window. */
    init_cpus();
    non_boot_lock = 1;
#endif /* CONFIG_MAX_NUM_NODES */

    if(is_hyp_mode()){
        printf("Enabling hypervisor MMU and paging\n");
        arm_enable_hyp_mmu();
    }
    printf("Enabling MMU and paging\n");
    arm_enable_mmu();

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
