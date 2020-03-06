/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <types.h>

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

#define PAGE_BITS           12

#define BIT(x)              (1 << (x))
#define MASK(n)             (BIT(n) - 1)
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define IS_ALIGNED(n, b)    (!((n) & MASK(b)))
#define ROUND_UP(n, b)      (((((n) - 1) >> (b)) + 1) << (b))
#define ROUND_DOWN(n, b) (((n) >> (b)) << (b))
#define ALIGN(n)            __attribute__((__aligned__(n)))
#if __has_attribute(externally_visible)
#define VISIBLE             __attribute__((externally_visible))
#else
#define VISIBLE
#endif
#define UNUSED              __attribute__((unused))
#define ARRAY_SIZE(a)       (sizeof(a)/sizeof((a)[0]))
#define NULL                ((void *)0)

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
    uintptr_t phys_virt_offset;
};

extern struct image_info kernel_info;
extern struct image_info user_info;
extern void *dtb;

/* Symbols defined in linker scripts. */
extern char _text[];
extern char _end[];
extern char _archive_start[];
extern char _archive_start_end[];

/* Clear BSS. */
void clear_bss(void);

/* Load images. */
void load_images(struct image_info *kernel_info, struct image_info *user_info,
                 int max_user_images, int *num_images, void *bootloader_dtb, void **chosen_dtb,
                 uint32_t *chosen_dtb_size);

/* Platform functions */
void platform_init(void);
void init_cpus(void);
int plat_console_putchar(unsigned int c);
