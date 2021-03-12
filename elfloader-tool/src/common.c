/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>

#include <printf.h>
#include <types.h>
#include <strops.h>
#include <binaries/elf/elf.h>
#include <cpio/cpio.h>

#include <elfloader.h>
#include <fdt.h>

#ifdef CONFIG_HASH_SHA
#include "crypt_sha256.h"
#elif CONFIG_HASH_MD5
#include "crypt_md5.h"
#endif

#include "hash.h"

#ifdef CONFIG_ELFLOADER_ROOTSERVERS_LAST
#include <platform_info.h> // this provides memory_region
#endif

extern char _bss[];
extern char _bss_end[];
void clear_bss(void)
{
    char *start = _bss;
    char *end = _bss_end;
    while (start < end) {
        *start = 0;
        start++;
    }
}

#define KEEP_HEADERS_SIZE BIT(PAGE_BITS)

/* Determine if two intervals overlap. */
static int regions_overlap(
    uintptr_t startA,
    uintptr_t endA,
    uintptr_t startB,
    uintptr_t endB)
{
    if (endA < startB) {
        return 0;
    }
    if (endB < startA) {
        return 0;
    }
    return 1;
}

/*
 * Ensure that we are able to use the given physical memory range.
 *
 * We fail if the destination physical range overlaps us, or if it goes outside
 * the bounds of memory.
 */
static int ensure_phys_range_valid(
    paddr_t paddr_min,
    paddr_t paddr_max)
{
    /*
     * Ensure that the physical load address of the object we're loading (called
     * `name`) doesn't overwrite us.
     */
    if (regions_overlap(paddr_min,
                        paddr_max - 1,
                        (uintptr_t)_text,
                        (uintptr_t)_end - 1)) {
        printf("ERROR: image would overlap ELF-loader\n");
        return -1;
    }

    return 0;
}

/*
 * Unpack an ELF file to the given physical address.
 */
static int unpack_elf_to_paddr(
    void const *elf,
    paddr_t dest_paddr)
{
    int ret;
    uint64_t min_vaddr, max_vaddr;
    size_t image_size;

    uintptr_t phys_virt_offset;

    /* Get the memory bounds. Unlike most other functions, this returns 1 on
     * success and anything else is an error.
     */
    ret = elf_getMemoryBounds(elf, 0, &min_vaddr, &max_vaddr);
    if (ret != 1) {
        printf("ERROR: Could not get image size\n");
        return -1;
    }

    image_size = (size_t)(max_vaddr - min_vaddr);
    phys_virt_offset = dest_paddr - (paddr_t)min_vaddr;

    /* Zero out all memory in the region, as the ELF file may be sparse. */
    memset((char *)dest_paddr, 0, image_size);

    /* Load each segment in the ELF file. */
    for (unsigned int i = 0; i < elf_getNumProgramHeaders(elf); i++) {
        vaddr_t dest_vaddr;
        size_t data_size, data_offset;

        /* Skip segments that are not marked as being loadable. */
        if (elf_getProgramHeaderType(elf, i) != PT_LOAD) {
            continue;
        }

        /* Parse size/length headers. */
        dest_vaddr = elf_getProgramHeaderVaddr(elf, i);
        data_size = elf_getProgramHeaderFileSize(elf, i);
        data_offset = elf_getProgramHeaderOffset(elf, i);

        /* Load data into memory. */
        memcpy((char *)dest_vaddr + phys_virt_offset,
               (char *)elf + data_offset, data_size);
    }

    return 0;
}

/*
 * Load an ELF file into physical memory at the given physical address.
 *
 * Returns in 'next_phys_addr' the byte past the last byte of the physical
 * address used.
 */
static int load_elf(
    char const *name,
    void const *elf,
    paddr_t dest_paddr,
    struct image_info *info,
    int keep_headers,
    paddr_t *next_phys_addr,
    unsigned long size,
    char const *hash)
{
    int ret;
    uint64_t min_vaddr, max_vaddr;

    /* Print diagnostics. */
    printf("ELF-loading image '%s' to %p\n", name, dest_paddr);

    /* Get the memory bounds. Unlike most other functions, this returns 1 on
     * success and anything else is an error.
     */
    ret = elf_getMemoryBounds(elf, 0, &min_vaddr, &max_vaddr);
    if (ret != 1) {
        printf("ERROR: Could not get image bounds\n");
        return -1;
    }

    /* round up size to the end of the page next page */
    max_vaddr = ROUND_UP(max_vaddr, PAGE_BITS);
    size_t image_size = (size_t)(max_vaddr - min_vaddr);

    /* Ensure our starting physical address is aligned. */
    if (!IS_ALIGNED(dest_paddr, PAGE_BITS)) {
        printf("ERROR: Attempting to load ELF at unaligned physical address\n");
        return -1;
    }

    /* Ensure that the ELF file itself is 4-byte aligned in memory, so that
     * libelf can perform word accesses on it. */
    if (!IS_ALIGNED(dest_paddr, 2)) {
        printf("ERROR: Input ELF file not 4-byte aligned in memory\n");
        return -1;
    }

#ifdef CONFIG_HASH_NONE

    UNUSED_VARIABLE(size);
    UNUSED_VARIABLE(hash);

#else

    /* Get the binary file that contains the SHA256 Hash */
    unsigned long cpio_len = _archive_start_end - _archive_start;
    void const *file_hash = cpio_get_file(_archive_start,
                                          cpio_len,
                                          hash,
                                          NULL);
    uint8_t *print_hash_pointer = (uint8_t *)file_hash;

    /* If the file hash doesn't have a pointer, the file doesn't exist, so we
     * cannot confirm the file is what we expect.
     */
    if (file_hash == NULL) {
        printf("ERROR: hash file '%s' doesn't exist\n", hash);
        return -1;
    }

    hashes_t hashes;

#ifdef CONFIG_HASH_SHA
    int hash_len = 32;
    hashes.hash_type = SHA_256;
#else
    int hash_len = 16;
    hashes.hash_type = MD5;
#endif

    uint8_t calculated_hash[hash_len];

    /* Print the Hash for the user to see */
    printf("Hash from ELF File: ");
    print_hash(print_hash_pointer, hash_len);

    get_hash(hashes, elf, size, calculated_hash);

    /* Print the hash so the user can see they're the same or different */
    printf("Hash for ELF Input: ");
    print_hash(calculated_hash, hash_len);

    /* Check to make sure the hashes are the same */
    ret = strncmp((char *)file_hash, (char *)calculated_hash, hash_len);
    if (0 != ret) {
        printf("ERROR: Hashes are different\n");
        return -1;
    }

#endif  /* CONFIG_HASH_NONE */

    /* Print diagnostics. */
    printf("  paddr=[%p..%p]\n", dest_paddr, dest_paddr + image_size - 1);
    printf("  vaddr=[%p..%p]\n", (vaddr_t)min_vaddr, (vaddr_t)max_vaddr - 1);
    printf("  virt_entry=%p\n", (vaddr_t)elf_getEntryPoint(elf));

    /* Ensure the ELF file is valid. */
    ret = elf_checkFile(elf);
    if (0 != ret) {
        printf("ERROR: Invalid ELF file\n");
        return -1;
    }

    /* Ensure sane alignment of the image. */
    if (!IS_ALIGNED(min_vaddr, PAGE_BITS)) {
        printf("ERROR: Start of image is not 4K-aligned\n");
        return -1;
    }

    /* Ensure that we region we want to write to is sane. */
    ret = ensure_phys_range_valid(dest_paddr, dest_paddr + image_size);
    if (0 != ret) {
        printf("ERROR: Physical address range invalid\n");
        return -1;
    }

    /* Copy the data. */
    ret = unpack_elf_to_paddr(elf, dest_paddr);
    if (0 != ret) {
        printf("ERROR: Unpacking ELF to %p failed\n", dest_paddr);
        return -1;
    }

    /* Record information about the placement of the image. */
    info->phys_region_start = dest_paddr;
    info->phys_region_end = dest_paddr + image_size;
    info->virt_region_start = (vaddr_t)min_vaddr;
    info->virt_region_end = (vaddr_t)max_vaddr;
    info->virt_entry = (vaddr_t)elf_getEntryPoint(elf);
    info->phys_virt_offset = dest_paddr - (vaddr_t)min_vaddr;

    /* Round up the destination address to the next page */
    dest_paddr = ROUND_UP(dest_paddr + image_size, PAGE_BITS);

    if (keep_headers) {
        /* Put the ELF headers in this page */
        uint32_t phnum = elf_getNumProgramHeaders(elf);
        uint32_t phsize;
        paddr_t source_paddr;
        if (ISELF32(elf)) {
            phsize = ((struct Elf32_Header const *)elf)->e_phentsize;
            source_paddr = (paddr_t)elf32_getProgramHeaderTable(elf);
        } else {
            phsize = ((struct Elf64_Header const *)elf)->e_phentsize;
            source_paddr = (paddr_t)elf64_getProgramHeaderTable(elf);
        }
        /* We have no way of sharing definitions with the kernel so we just
         * memcpy to a bunch of magic offsets. Explicit numbers for sizes
         * and offsets are used so that it is clear exactly what the layout
         * is */
        memcpy((void *)dest_paddr, &phnum, 4);
        memcpy((void *)(dest_paddr + 4), &phsize, 4);
        memcpy((void *)(dest_paddr + 8), (void *)source_paddr, phsize * phnum);
        /* return the frame after our headers */
        dest_paddr += KEEP_HEADERS_SIZE;
    }

    if (next_phys_addr) {
        *next_phys_addr = dest_paddr;
    }
    return 0;
}

/*
 * ELF-loader for ARM systems.
 *
 * We are currently running out of physical memory, with an ELF file for the
 * kernel and one or more ELF files for the userspace image. (Typically there
 * will only be one userspace ELF file, though if we are running a multi-core
 * CPU, we may have multiple userspace images; one per CPU.) These ELF files
 * are packed into an 'ar' archive.
 *
 * The kernel ELF file indicates what physical address it wants to be loaded
 * at, while userspace images run out of virtual memory, so don't have any
 * requirements about where they are located. We place the kernel at its
 * desired location, and then load userspace images straight after it in
 * physical memory.
 *
 * Several things could possibly go wrong:
 *
 *  1. The physical load address of the kernel might want to overwrite this
 *  ELF-loader;
 *
 *  2. The physical load addresses of the kernel might not actually be in
 *  physical memory;
 *
 *  3. Userspace images may not fit in physical memory, or may try to overlap
 *  the ELF-loader.
 *
 *  We attempt to check for some of these, but some may go unnoticed.
 */
int load_images(
    struct image_info *kernel_info,
    struct image_info *user_info,
    unsigned int max_user_images,
    unsigned int *num_images,
    void *bootloader_dtb,
    void **chosen_dtb,
    uint32_t *chosen_dtb_size)
{
    int ret;
    uint64_t kernel_phys_start, kernel_phys_end;
    uintptr_t dtb_phys_start, dtb_phys_end;
    paddr_t next_phys_addr;
    const char *elf_filename;
    unsigned long kernel_filesize;
    int has_dtb_cpio = 0;

    /* Load kernel. */
    unsigned long cpio_len = _archive_start_end - _archive_start;
    void const *kernel_elf = cpio_get_file(_archive_start,
                                           cpio_len,
                                           "kernel.elf",
                                           &kernel_filesize);
    if (kernel_elf == NULL) {
        printf("ERROR: No kernel image present in archive\n");
        return -1;
    }

    ret = elf_checkFile(kernel_elf);
    if (ret != 0) {
        printf("ERROR: Kernel image not a valid ELF file\n");
        return -1;
    }

    /* Get physical memory bounds. Unlike most other functions, this returns 1
     * on success and anything else is an error.
     */
    ret = elf_getMemoryBounds(kernel_elf, 1, &kernel_phys_start,
                              &kernel_phys_end);
    if (1 != ret) {
        printf("ERROR: Could not get kernel memory bounds\n");
        return -1;
    }

    void const *dtb = NULL;

#ifdef CONFIG_ELFLOADER_INCLUDE_DTB

    if (chosen_dtb) {
        printf("Looking for DTB in CPIO archive...");
        /*
         * Note the lack of newline in the above printf().  Normally one would
         * have an fflush(stdout) here to ensure that the message shows up on a
         * line-buffered stream (which is the POSIX default on terminal
         * devices).  But we are freestanding (on the "bare metal"), and using
         * our own unbuffered printf() implementation.
         */
        dtb = cpio_get_file(_archive_start, cpio_len, "kernel.dtb", NULL);
        if (dtb == NULL) {
            printf("not found.\n");
        } else {
            has_dtb_cpio = 1;
            printf("found at %p.\n", dtb);
        }
    }

#endif /* CONFIG_ELFLOADER_INCLUDE_DTB */

    if (chosen_dtb && !dtb && bootloader_dtb) {
        /* Use the bootloader's DTB if we are not using the DTB in the CPIO
         * archive.
         */
        dtb = bootloader_dtb;
    }

    /*
     * Move the DTB out of the way, if it's present.
     */
    if (dtb) {
        /* keep it page aligned */
        next_phys_addr = dtb_phys_start = ROUND_UP(kernel_phys_end, PAGE_BITS);

        *chosen_dtb_size = fdt_size(dtb);
        if (!*chosen_dtb_size) {
            printf("ERROR: Invalid device tree blob supplied\n");
            return -1;
        }

        /* Make sure this is a sane thing to do */
        ret = ensure_phys_range_valid(next_phys_addr,
                                      next_phys_addr + *chosen_dtb_size);
        if (0 != ret) {
            printf("ERROR: Physical address of DTB invalid\n");
            return -1;
        }

        memmove((void *)next_phys_addr, dtb, *chosen_dtb_size);
        next_phys_addr += *chosen_dtb_size;
        next_phys_addr = ROUND_UP(next_phys_addr, PAGE_BITS);
        dtb_phys_end = next_phys_addr;

        printf("Loaded DTB from %p.\n", dtb);
        printf("   paddr=[%p..%p]\n", dtb_phys_start, dtb_phys_end - 1);
        *chosen_dtb = (void *)dtb_phys_start;
    } else {
        next_phys_addr = ROUND_UP(kernel_phys_end, PAGE_BITS);
    }

    /* Load the kernel */
    ret = load_elf("kernel",
                   kernel_elf,
                   (paddr_t)kernel_phys_start,
                   kernel_info,
                   0, // don't keep ELF headers
                   NULL, // we have calculated next_phys_addr already
                   kernel_filesize,
                   "kernel.bin");

    if (0 != ret) {
        printf("ERROR: Could not load kernel ELF\n");
        return -1;
    }

    /*
     * Load userspace images.
     *
     * We assume (and check) that the kernel is the first file in the archive,
     * that the DTB is the second if present,
     * and then load the (n+user_elf_offset)'th file in the archive onto the
     * (n)'th CPU.
     */
    unsigned int user_elf_offset = 2;
    cpio_get_entry(_archive_start, cpio_len, 0, &elf_filename, NULL);
    ret = strcmp(elf_filename, "kernel.elf");
    if (0 != ret) {
        printf("ERROR: Kernel image not first image in archive\n");
        return -1;
    }
    cpio_get_entry(_archive_start, cpio_len, 1, &elf_filename, NULL);
    ret = strcmp(elf_filename, "kernel.dtb");
    if (0 != ret) {
        if (has_dtb_cpio) {
            printf("ERROR: Kernel DTB not second image in archive\n");
            return -1;
        }
        user_elf_offset = 1;
    }

#ifdef CONFIG_ELFLOADER_ROOTSERVERS_LAST

    /* work out the size of the user images - this corresponds to how much
     * memory load_elf uses */
    unsigned int total_user_image_size = 0;
    for (unsigned int i = 0; i < max_user_images; i++) {
        void const *user_elf = cpio_get_entry(_archive_start,
                                              cpio_len,
                                              i + user_elf_offset,
                                              NULL,
                                              NULL);
        if (user_elf == NULL) {
            break;
        }
        /* Get the memory bounds. Unlike most other functions, this returns 1 on
         * success and anything else is an error.
         */
        uint64_t min_vaddr, max_vaddr;
        int ret = elf_getMemoryBounds(elf, 0, &min_vaddr, &max_vaddr);
        if (ret != 1) {
            printf("ERROR: Could not get image bounds\n");
            return -1;
        }
        /* round up size to the end of the page next page */
        total_user_image_size += (ROUND_UP(max_vaddr, PAGE_BITS) - min_vaddr)
                                 + KEEP_HEADERS_SIZE;
    }

    /* work out where to place the user image */

    next_phys_addr = ROUND_DOWN(memory_region[0].end, PAGE_BITS)
                     - ROUND_UP(total_user_image_size, PAGE_BITS);

#endif /* CONFIG_ELFLOADER_ROOTSERVERS_LAST */

    *num_images = 0;
    for (unsigned int i = 0; i < max_user_images; i++) {
        /* Fetch info about the next ELF file in the archive. */
        unsigned long elf_filesize = 0;
        void const *user_elf = cpio_get_entry(_archive_start,
                                              cpio_len,
                                              i + user_elf_offset,
                                              &elf_filename,
                                              &elf_filesize);
        if (user_elf == NULL) {
            break;
        }

        /* Load the file into memory. */
        ret = load_elf(elf_filename,
                       user_elf,
                       next_phys_addr,
                       &user_info[*num_images],
                       1,  // keep ELF headers
                       &next_phys_addr,
                       elf_filesize,
                       "app.bin");
        if (0 != ret) {
            printf("ERROR: Could not load user image ELF\n");
        }

        *num_images = i + 1;
    }

    return 0;
}

void __attribute__((weak)) platform_init(void)
{
    /* nothing by default */
}
