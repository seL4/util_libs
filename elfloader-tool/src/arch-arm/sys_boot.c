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
#include <elfloader/gen_config.h>

#include <printf.h>
#include <types.h>
#include <abort.h>
#include <strops.h>
#include <cpuid.h>
#include <platform.h>

#include <binaries/efi/efi.h>
#include <elfloader.h>

/* 0xd00dfeed in big endian */
#define DTB_MAGIC (0xedfe0dd0)

ALIGN(BIT(PAGE_BITS)) VISIBLE
char core_stack_alloc[CONFIG_MAX_NUM_NODES][BIT(PAGE_BITS)];

struct image_info kernel_info;
struct image_info user_info;
void *dtb;
uint32_t dtb_size;

extern void finish_relocation(int offset);
void continue_boot(void);

/*
 * Make sure the ELF loader is below the kernel's first virtual address
 * so that when we enable the MMU we can keep executing.
 */
void relocate_below_kernel(void)
{
    /*
     * These are the ELF loader's physical addresses,
     * since we are either running with MMU off or
     * identity-mapped.
     */
    uintptr_t start = (uintptr_t)_start;
    uintptr_t end = (uintptr_t)_end;

    if (end <= kernel_info.virt_region_start) {
        /*
         * If the ELF loader is already below the kernel,
         * skip relocation.
         */
        continue_boot();
        return;
    }

    /*
     * Note: we make the (potentially incorrect) assumption
     * that there is enough physical RAM below the kernel's first vaddr
     * to fit the ELF loader.
     * FIXME: do we need to make sure we don't accidentally wipe out the DTB too?
     */
    uintptr_t size = end - start;

    /*
     * we ROUND_UP size in this calculation so that all page-aligned things
     * (interrupt vectors, stack, etc.) end up in other page-aligned locations.
     */
    uintptr_t new_base = kernel_info.virt_region_start - (ROUND_UP(size, PAGE_BITS)) - (1 << PAGE_BITS);
    uint32_t offset = start - new_base;
    printf("relocating from %p-%p to %p-%p... size=%x\n", start, end, new_base, new_base + size, size);

    memmove((void *)new_base, (void *)start, size);

    /* call into assembly to do the finishing touches */
    finish_relocation(offset);
}

/*
 * Entry point.
 *
 * Unpack images, setup the MMU, jump to the kernel.
 */
void main(UNUSED void *arg)
{
    int num_apps;

    void *bootloader_dtb = NULL;

#ifdef CONFIG_IMAGE_UIMAGE
    if (arg) {
        uint32_t magic = *(uint32_t *)arg;
        /*
         * This might happen on ancient bootloaders which
         * still think Linux wants atags instead of a
         * device tree.
         */
        if (magic != DTB_MAGIC) {
            printf("Bootloader did not supply a valid device tree!\n");
            arg = NULL;
        }
    }
    bootloader_dtb = arg;
#else
    bootloader_dtb = NULL;
#endif

#ifdef CONFIG_IMAGE_EFI
    if (efi_exit_boot_services() != EFI_SUCCESS) {
        printf("Unable to exit UEFI boot services!\n");
        abort();
    }

    bootloader_dtb = efi_get_fdt();
#endif

    /* Print welcome message. */
    platform_init();
    printf("\nELF-loader started on ");
    print_cpuid();

    printf("  paddr=[%p..%p]\n", _start, _end - 1);

    /*
     * U-Boot will either pass us a DTB, or (if we're being booted via bootelf)
     * pass '0' in argc.
     */
    if (bootloader_dtb) {
        printf("  dtb=%p\n", dtb);
    } else {
        printf("No DTB passed in from boot loader.\n");
    }

    /* Unpack ELF images into memory. */
    load_images(&kernel_info, &user_info, 1, &num_apps, bootloader_dtb, &dtb, &dtb_size);
    if (num_apps != 1) {
        printf("No user images loaded!\n");
        abort();
    }

    /*
     * We don't really know where we've been loaded.
     * It's possible that EFI loaded us in a place
     * that will become part of the 'kernel window'
     * once we switch to the boot page tables.
     * Make sure this is not the case.
     */
    relocate_below_kernel();
    printf("Relocation failed, aborting.\n");
    abort();
}

void continue_boot()
{
    printf("ELF loader relocated, continuing boot...\n");

#if (defined(CONFIG_ARCH_ARM_V7A) || defined(CONFIG_ARCH_ARM_V8A)) && !defined(CONFIG_ARM_HYPERVISOR_SUPPORT)
    if (is_hyp_mode()) {
        extern void leave_hyp(void);
        leave_hyp();
    }
#endif
    /* Setup MMU. */
    if (is_hyp_mode()) {
#ifdef CONFIG_ARCH_AARCH64
        extern void disable_caches_hyp();
        disable_caches_hyp();
#endif
        init_hyp_boot_vspace(&kernel_info);
    } else {
        /* If we are not in HYP mode, we enable the SV MMU and paging
         * just in case the kernel does not support hyp mode. */
        init_boot_vspace(&kernel_info);
    }

#if CONFIG_MAX_NUM_NODES > 1
    smp_boot();
#endif /* CONFIG_MAX_NUM_NODES */

    if (is_hyp_mode()) {
        printf("Enabling hypervisor MMU and paging\n");
        arm_enable_hyp_mmu();
    } else {
        printf("Enabling MMU and paging\n");
        arm_enable_mmu();
    }

    /* Enter kernel. */
    if (UART_PPTR < kernel_info.virt_region_start) {
        printf("Jumping to kernel-image entry point...\n\n");
    } else {
        /* Our serial port is no longer accessible */
    }

    ((init_arm_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                                user_info.phys_region_end, user_info.phys_virt_offset,
                                                user_info.virt_entry, (paddr_t)dtb, dtb_size);

    /* We should never get here. */
    printf("Kernel returned back to the elf-loader.\n");
    abort();
}
