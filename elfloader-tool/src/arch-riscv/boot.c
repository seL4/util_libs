/*
 * Copyright 2018, Data61
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
#include <binaries/elf/elf.h>
#include <elfloader.h>
#include <platform.h>
#include <abort.h>
#include <cpio/cpio.h>


#define PTE_TYPE_TABLE 0x00
#define PTE_TYPE_SRWX 0xCE

#define RISCV_PGSHIFT 12
#define RISCV_PGSIZE (1 << RISCV_PGSHIFT)

// page table entry (PTE) field
#define PTE_V     0x001 // Valid

#define PTE64_PPN0_SHIFT 10

#define PTES_PER_PT (RISCV_PGSIZE/sizeof(long))


#define PTE64_CREATE(PPN, TYPE) (unsigned long) (((uint32_t)PPN) | (TYPE) | PTE_V)
#define PTE64_PT_CREATE(PT_BASE) \
  (((unsigned long)((unsigned long) PT_BASE) / RISCV_PGSIZE) << 10 | PTE_TYPE_TABLE | PTE_V)

struct image_info kernel_info;
struct image_info user_info;

unsigned long l1pt[PTES_PER_PT] __attribute__((aligned(4096)));
char elfloader_stack_alloc[BIT(CONFIG_KERNEL_STACK_BITS)];

void
map_kernel_window(struct image_info *kernel_info)
{
    uint32_t i;
    // first create a complete set of 1-to-1 mappings for all of memory. this is a brute
    // force way to ensure this elfloader is mapped into the new address space
    for(i = 0; i < PTES_PER_PT; i++) {
#if __riscv_xlen == 32
        l1pt[i] = PTE64_CREATE((uint64_t)(i << 20), PTE_TYPE_SRWX);
#else
        l1pt[i] = PTE64_CREATE((uint64_t)(i << 28), PTE_TYPE_SRWX);
#endif
    }
    //Now create any neccessary entries for the kernel vaddr->paddr
    i = (kernel_info->virt_region_start >> ((CONFIG_PT_LEVELS - 1) * (__riscv_xlen == 32 ? 10 : 9) + 12)) % PTES_PER_PT;
    for (int page = 0; i < PTES_PER_PT; i++, page++) {
#if __riscv_xlen == 32
         l1pt[i] = PTE64_CREATE((uint32_t)((kernel_info->phys_region_start >> 12) + page) << 10, PTE_TYPE_SRWX);
#else
         l1pt[i] = PTE64_CREATE((uint64_t)((kernel_info->phys_region_start >> 12) + page) << PTE64_PPN0_SHIFT, PTE_TYPE_SRWX);
#endif
    }
}

#if __riscv_xlen == 32
#define LW lw
#else
#define LW ld
#endif

#if CONFIG_PT_LEVELS == 2
    uint64_t vm_mode = 0x1llu << 31;
#elif CONFIG_PT_LEVELS == 3
    uint64_t vm_mode = 0x8llu << 60;
#elif CONFIG_PT_LEVELS == 4
    uint64_t vm_mode = 0x9llu << 60;
#else
#error "Wrong PT level"
#endif

int num_apps = 0;
void main(int hardid, unsigned long dtb)
{
    (void) hardid;
    printf("ELF-loader started on\n");

    printf("  paddr=[%p..%p]\n", _start, _end - 1);
    /* Unpack ELF images into memory. */
    load_images(&kernel_info, &user_info, 1, &num_apps);
    if (num_apps != 1) {
        printf("No user images loaded!\n");
        abort();
    }

    map_kernel_window(&kernel_info);

    printf("Jumping to kernel-image entry point...\n\n");

    asm volatile("sfence.vma");

    asm volatile(
        "csrw sptbr, %0\n"
       :
       : "r" (vm_mode | (uintptr_t)l1pt >> RISCV_PGSHIFT)
       :
   );

    ((init_riscv_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                            user_info.phys_region_end, user_info.phys_virt_offset,
                                            user_info.virt_entry, 0, dtb);

  /* We should never get here. */
    printf("Kernel returned back to the elf-loader.\n");
}
