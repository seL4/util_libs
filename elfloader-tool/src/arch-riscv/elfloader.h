/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _ELFLOADER_H_
#define _ELFLOADER_H_

#include <types.h>

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

#define PAGE_BITS 12

#define BIT(x) (1 << (x))
#define MASK(n) (BIT(n)-1)
#define IS_ALIGNED(n, b) (!((n) & MASK(b)))
#define ROUND_UP(n, b) (((((n) - 1) >> (b)) + 1) << (b))

/*
 * Information about an image we are loading.
 */
struct image_info {
    /* Start/end byte of the image in physical memory. */
    paddr_t phys_region_start;
    paddr_t phys_region_end;

    /* Start/end byte in virtual memory the image requires to be located. */
    vaddr_t virt_region_start;
    vaddr_t virt_region_end;

    /* Virtual address of the user image's entry point. */
    vaddr_t  virt_entry;

    /*
     * Offset between the physical/virtual addresses of the image.
     *
     * In particular:
     *
     *  virtual_address + phys_virt_offset = physical_address
     */
    uint64_t phys_virt_offset;
};

/* Symbols defined in linker scripts. */
extern char _start[];
extern char _end[];
extern char _archive_start[];
extern char _archive_end[];
extern uint32_t _boot_pd[];
extern uint64_t _lpae_boot_pgd[];
extern uint64_t _lpae_boot_pmd[];

/* Load images. */
void load_images(struct image_info *kernel_info, struct image_info *user_info,
                 int max_user_images, int *num_images);

/* Platform functions */
void platform_init(void);
void init_cpus(void);

#endif /* _ELFLOADER_H_ */

