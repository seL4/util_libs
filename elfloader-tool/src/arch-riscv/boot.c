/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * Copyright 2018, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_GPL)
 */
#include <autoconf.h>
#include <elfloader/gen_config.h>

#include <types.h>
#include <binaries/elf/elf.h>
#include <elfloader.h>
#include <abort.h>
#include <cpio/cpio.h>

#define PT_LEVEL_1 1
#define PT_LEVEL_2 2

#define PT_LEVEL_1_BITS 30
#define PT_LEVEL_2_BITS 21

#define PTE_TYPE_TABLE 0x00
#define PTE_TYPE_SRWX 0xCE

#define RISCV_PGSHIFT 12
#define RISCV_PGSIZE BIT(RISCV_PGSHIFT)

// page table entry (PTE) field
#define PTE_V     0x001 // Valid

#define PTE_PPN0_SHIFT 10

#if __riscv_xlen == 32
#define PT_INDEX_BITS  10
#else
#define PT_INDEX_BITS  9
#endif

#define PTES_PER_PT BIT(PT_INDEX_BITS)

#define PTE_CREATE_PPN(PT_BASE)  (unsigned long)(((PT_BASE) >> RISCV_PGSHIFT) << PTE_PPN0_SHIFT)
#define PTE_CREATE_NEXT(PT_BASE) (unsigned long)(PTE_CREATE_PPN(PT_BASE) | PTE_TYPE_TABLE | PTE_V)
#define PTE_CREATE_LEAF(PT_BASE) (unsigned long)(PTE_CREATE_PPN(PT_BASE) | PTE_TYPE_SRWX | PTE_V)

#define GET_PT_INDEX(addr, n) (((addr) >> (((PT_INDEX_BITS) * ((CONFIG_PT_LEVELS) - (n))) + RISCV_PGSHIFT)) % PTES_PER_PT)

#define VIRT_PHYS_ALIGNED(virt, phys, level_bits) (IS_ALIGNED((virt), (level_bits)) && IS_ALIGNED((phys), (level_bits)))

struct image_info kernel_info;
struct image_info user_info;

unsigned long l1pt[PTES_PER_PT] __attribute__((aligned(4096)));
#if __riscv_xlen == 64
unsigned long l2pt[PTES_PER_PT] __attribute__((aligned(4096)));
unsigned long l2pt_elf[PTES_PER_PT] __attribute__((aligned(4096)));
#endif

char elfloader_stack_alloc[BIT(CONFIG_KERNEL_STACK_BITS)];

void map_kernel_window(struct image_info *kernel_info)
{
    uint32_t index;
    unsigned long *lpt;

    /* Map the elfloader into the new address space */
    index = GET_PT_INDEX((uintptr_t)_start, PT_LEVEL_1);

#if __riscv_xlen == 32
    lpt = l1pt;
#else
    lpt = l2pt_elf;
    l1pt[index] = PTE_CREATE_NEXT((uintptr_t)l2pt_elf);
    index = GET_PT_INDEX((uintptr_t)_start, PT_LEVEL_2);
#endif

    if (IS_ALIGNED((uintptr_t)_start, PT_LEVEL_2_BITS)) {
        for (int page = 0; index < PTES_PER_PT; index++, page++) {
            lpt[index] = PTE_CREATE_LEAF((uintptr_t)_start +
                                         (page << PT_LEVEL_2_BITS));
        }
    } else {
        printf("Elfloader not properly aligned\n");
        abort();
    }

    /* Map the kernel into the new address space */
    index = GET_PT_INDEX(kernel_info->virt_region_start, PT_LEVEL_1);

#if __riscv_xlen == 64
    lpt = l2pt;
    l1pt[index] = PTE_CREATE_NEXT((uintptr_t)l2pt);
    index = GET_PT_INDEX(kernel_info->virt_region_start, PT_LEVEL_2);
#endif
    if (VIRT_PHYS_ALIGNED(kernel_info->virt_region_start,
                          kernel_info->phys_region_start, PT_LEVEL_2_BITS)) {
        for (int page = 0; index < PTES_PER_PT; index++, page++) {
            lpt[index] = PTE_CREATE_LEAF(kernel_info->phys_region_start +
                                         (page << PT_LEVEL_2_BITS));
        }
    } else {
        printf("Kernel not properly aligned\n");
        abort();
    }
}

#if CONFIG_PT_LEVELS == 2
uint64_t vm_mode = 0x1llu << 31;
#elif CONFIG_PT_LEVELS == 3
uint64_t vm_mode = 0x8llu << 60;
#elif CONFIG_PT_LEVELS == 4
uint64_t vm_mode = 0x9llu << 60;
#else
#error "Wrong PT level"
#endif

#if CONFIG_MAX_NUM_NODES > 1
int secondary_go = 0;
int next_logical_core_id = 1;
int mutex = 0;
int core_ready[CONFIG_MAX_NUM_NODES] = { 0 };
static void set_and_wait_for_ready(int hart_id, int core_id)
{
    while (__atomic_exchange_n(&mutex, 1, __ATOMIC_ACQUIRE) != 0);
    printf("Hart ID %d core ID %d\n", hart_id, core_id);
    core_ready[core_id] = 1;
    __atomic_store_n(&mutex, 0, __ATOMIC_RELEASE);

    for (int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        while (__atomic_load_n(&core_ready[i], __ATOMIC_RELAXED) == 0) ;
    }
}
#endif

static inline void sfence_vma(void)
{
    asm volatile("sfence.vma" ::: "memory");
}

static inline void ifence(void)
{
    asm volatile("fence.i" ::: "memory");
}

static inline void enable_virtual_memory(void)
{
    sfence_vma();
    asm volatile(
        "csrw sptbr, %0\n"
        :
        : "r"(vm_mode | (uintptr_t)l1pt >> RISCV_PGSHIFT)
        :
    );
    ifence();
}

int num_apps = 0;
void main(UNUSED int hardid, void *bootloader_dtb)
{
    void *dtb = NULL;
    uint32_t dtb_size = 0;
    printf("ELF-loader started on (HART %d) (NODES %d)\n", hartid, CONFIG_MAX_NUM_NODES);

    printf("  paddr=[%p..%p]\n", _start, _end - 1);
    /* Unpack ELF images into memory. */
    load_images(&kernel_info, &user_info, 1, &num_apps, bootloader_dtb, &dtb, &dtb_size);
    if (num_apps != 1) {
        printf("No user images loaded!\n");
        abort();
    }

    map_kernel_window(&kernel_info);

    printf("Jumping to kernel-image entry point...\n\n");

#if CONFIG_MAX_NUM_NODES > 1
    /* Unleash secondary cores */
    __atomic_store_n(&secondary_go, 1, __ATOMIC_RELEASE);
    /* Set that the current core is ready and wait for other cores */
    set_and_wait_for_ready(hartid, 0);
#endif

    enable_virtual_memory();

    /* TODO: pass DTB to kernel. */
    ((init_riscv_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                                  user_info.phys_region_end, user_info.phys_virt_offset,
                                                  user_info.virt_entry
#if CONFIG_MAX_NUM_NODES > 1
                                                  ,
                                                  hartid,
                                                  0
#endif
                                                 );

    /* We should never get here. */
    printf("Kernel returned back to the elf-loader.\n");
}

#if CONFIG_MAX_NUM_NODES > 1

void secondary_entry(int hart_id, int core_id)
{
    while (__atomic_load_n(&secondary_go, __ATOMIC_ACQUIRE) == 0) ;

    set_and_wait_for_ready(hart_id, core_id);

    enable_virtual_memory();

    /* TODO: pass DTB to kernel. */
    ((init_riscv_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                                  user_info.phys_region_end, user_info.phys_virt_offset,
                                                  user_info.virt_entry
                                                  ,
                                                  hart_id,
                                                  core_id
                                                 );
}

#endif
