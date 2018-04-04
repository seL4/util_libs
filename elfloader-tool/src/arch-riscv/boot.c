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
#define PTE_TYPE_TABLE_GLOBAL 0x02
#define PTE_TYPE_URX_SR 0x04
#define PTE_TYPE_URWX_SRW 0x06
#define PTE_TYPE_UR_SR 0x08
#define PTE_TYPE_URW_SRW 0x0A
#define PTE_TYPE_URX_SRX 0x0C
#define PTE_TYPE_URWX_SRWX 0x0E
#define PTE_TYPE_SR 0x10
#define PTE_TYPE_SRW 0x12
#define PTE_TYPE_SRX 0x14
#define PTE_TYPE_SRWX 0xCE
#define PTE_TYPE_SR_GLOBAL 0x18
#define PTE_TYPE_SRW_GLOBAL 0x1A
#define PTE_TYPE_SRX_GLOBAL 0x1C
#define PTE_TYPE_SRWX_GLOBAL 0x1E

#define RISCV_PGSHIFT 12
#define RISCV_PGSIZE (1 << RISCV_PGSHIFT)

// page table entry (PTE) fields
#define PTE_V     0x001 // Valid
#define PTE_TYPE  0x01E // Type
#define PTE_R     0x020 // Referenced
#define PTE_D     0x040 // Dirty
#define PTE_SOFT  0x380 // Reserved for Software

#define PTE_PPN_SHIFT 10
#define PTE_PPN1_SHIFT 20

#define PTE64_PPN2_SHIFT 28
#define PTE64_PPN1_SHIFT 19
#define PTE64_PPN0_SHIFT 10

#define PTES_PER_PT (RISCV_PGSIZE/sizeof(long))

/* Virtual address to index conforming sv32 PTE format */
#define VIRT1_TO_IDX(addr) ((addr) >> 22)
#define VIRT0_TO_IDX(addr) (((addr) >> 12)

#define SV39_VIRT_TO_VPN2(addr) ((addr) >> 30)
#define SV39_VIRT_TO_VPN1(addr) ((addr) >> 21)
#define SV39_VIRT_TO_VPN0(addr) ((addr) >> 12)

#define PTE_CREATE(PPN, TYPE) (((PPN) << PTE_PPN_SHIFT) | (TYPE) | PTE_V)
#define PTE64_CREATE(PPN, TYPE) (unsigned long) (((uint32_t)PPN) | (TYPE) | PTE_V)
#define PTE64_PT_CREATE(PT_BASE) \
  (((unsigned long)((unsigned long) PT_BASE) / RISCV_PGSIZE) << 10 | PTE_TYPE_TABLE | PTE_V)

#define write_csr(reg, val) \
  asm volatile ("csrw " #reg ", %0" :: "r"(val))

#define swap_csr(reg, val) ({ long __tmp; asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "r"(val)); \
  __tmp; })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
  asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
  asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
  asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

struct image_info kernel_info;
struct image_info user_info;

unsigned long l1pt[PTES_PER_PT] __attribute__((aligned(4096)));
unsigned long l2pt_elfloader[PTES_PER_PT] __attribute__((aligned(4096)));
unsigned long l3pt_elfloader[PTES_PER_PT] __attribute__((aligned(4096)));
unsigned long l2pt_kernel[PTES_PER_PT] __attribute__((aligned(4096)));
char elfloader_stack_alloc[BIT(CONFIG_KERNEL_STACK_BITS)];

void
map_kernel_window(struct image_info *kernel_info)
{
    uint32_t i;
    // paddr_t  phys;
    // phys = SV39_VIRT_TO_VPN1(kernel_info->phys_region_start) & 0x1FF;

   for(i = 0; i < 20; i++)
 #if __riscv_xlen == 32
      l1pt[513] = PTE64_CREATE((uint64_t)(513 << 20), PTE_TYPE_SRWX);
 #else
     /* Sv39 - 1 GiB mappings */
     l1pt[i] = PTE64_CREATE((uint64_t)(i << 28), PTE_TYPE_SRWX);
 #endif

#if __riscv_xlen == 32
     l1pt[512] = PTE64_CREATE((uint32_t)(kernel_info->phys_region_start >> 12) << 10, PTE_TYPE_SRWX);
#else
     l1pt[510] = PTE64_CREATE((uint64_t)(kernel_info->phys_region_start >> 12) << PTE64_PPN0_SHIFT, PTE_TYPE_SRWX);
     l1pt[256] = PTE64_CREATE((uint64_t)(kernel_info->phys_region_start >> 12) << PTE64_PPN0_SHIFT, PTE_TYPE_SRWX);
#endif

    l1pt[0] = PTE64_PT_CREATE((unsigned long)(&l2pt_elfloader));
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
