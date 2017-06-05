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

#include <types.h>
#include <elfloader.h>
#include <mode/structures.h>

/*
* Create a "boot" page table, which contains a 1:1 mapping below
* the kernel's first vaddr, and a virtual-to-physical mapping above the
* kernel's first vaddr.
*/
void init_boot_vspace(struct image_info *kernel_info)
{
    word_t i;

    vaddr_t first_vaddr = kernel_info->virt_region_start;
    paddr_t first_paddr = kernel_info->phys_region_start;

    _boot_pgd_down[0] = ((uintptr_t)_boot_pud_down) | BIT(1) | BIT(0); /* its a page table */
    
    for(i = 0; i < BIT(PUD_BITS); i++) {
        _boot_pud_down[i] = (i << ARM_1GB_BLOCK_BITS)
                            | BIT(10) /* access flag */
                            | (0 << 2) /* strongly ordered memory */
                            | BIT(0); /* 1G block */
    }

    _boot_pgd_up[GET_PGD_INDEX(first_vaddr)]
        = ((uintptr_t)_boot_pud_up) | BIT(1) | BIT(0); /* its a page table */

    _boot_pud_up[GET_PUD_INDEX(first_vaddr)]
        = ((uintptr_t)_boot_pmd_up) | BIT(1) | BIT(0); /* its a page table */

    for (i = 0; i < BIT(PMD_BITS); i++) {
        _boot_pmd_up[i] = ((i << ARM_2MB_BLOCK_BITS) + first_paddr)
                          | BIT(10) /* access flag */
                          | (4 << 2) /* MT_NORMAL memory */
                          | BIT(0); /* 2M block */
    }
}

void init_hyp_boot_vspace(struct image_info *kernel_info)
{
    /* 
     * The paging for EL2 has not been implemented yet!
     * Therefore, we drop to EL1 and currently kernel cannot run in 64-bit Hyp mode.
     */
    (void)kernel_info;
}
