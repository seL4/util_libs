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

#define KEEP_HEADERS_SIZE BIT(PAGE_BITS)

/* Determine if two intervals overlap. */
static int regions_overlap(uintptr_t startA, uintptr_t endA,
                           uintptr_t startB, uintptr_t endB)
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
 * We abort if the destination physical range overlaps us, or if it
 * goes outside the bounds of memory.
 */
static void ensure_phys_range_valid(char const *const name, paddr_t paddr_min,
                                    paddr_t paddr_max)
{
    /*
     * Ensure that the physical load address of the object we're loading (called
     * `name`) doesn't overwrite us.
     */
    if (regions_overlap(paddr_min, paddr_max - 1, (word_t)_start, (word_t)_end - 1)) {
        printf("%s load address would overlap ELF-loader!\n", name);
        abort();
    }
}

/*
 * Unpack an ELF file to the given physical address.
 */
static void unpack_elf_to_paddr(void *elf, paddr_t dest_paddr)
{
    uint16_t i;
    uint64_t min_vaddr, max_vaddr;
    size_t image_size;

    word_t phys_virt_offset;

    /* Get size of the image. */
    elf_getMemoryBounds(elf, 0, &min_vaddr, &max_vaddr);
    image_size = (size_t)(max_vaddr - min_vaddr);
    phys_virt_offset = dest_paddr - (paddr_t)min_vaddr;

    /* Zero out all memory in the region, as the ELF file may be sparse. */
    memset((char *)dest_paddr, 0, image_size);

    /* Load each segment in the ELF file. */
    for (i = 0; i < elf_getNumProgramHeaders(elf); i++) {
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
}

static size_t rounded_image_size(void *elf, uint64_t *min_vaddr, uint64_t *max_vaddr)
{
    elf_getMemoryBounds(elf, 0, min_vaddr, max_vaddr);
    *max_vaddr = ROUND_UP(*max_vaddr, PAGE_BITS);
    return (size_t)(*max_vaddr - *min_vaddr);
}

/*
 * Load an ELF file into physical memory at the given physical address.
 *
 * Return the byte past the last byte of the physical address used.
 */
static paddr_t load_elf(const char *name, void *elf, paddr_t dest_paddr,
                        struct image_info *info, int keep_headers,
                        __attribute__((unused)) unsigned long size,
                        __attribute__((unused)) const char *hash)
{
    uint64_t min_vaddr, max_vaddr;
    /* Fetch image info. */
    size_t image_size = rounded_image_size(elf, &min_vaddr, &max_vaddr);

    /* Ensure our starting physical address is aligned. */
    if (!IS_ALIGNED(dest_paddr, PAGE_BITS)) {
        printf("Attempting to load ELF at unaligned physical address!\n");
        abort();
    }

    /* Ensure that the ELF file itself is 4-byte aligned in memory, so that
     * libelf can perform word accesses on it. */
    if (!IS_ALIGNED(dest_paddr, 2)) {
        printf("Input ELF file not 4-byte aligned in memory!\n");
        abort();
    }

#ifndef CONFIG_HASH_NONE

    /* Get the binary file that contains the SHA256 Hash */
    unsigned long unused;
    unsigned long cpio_len = _archive_start_end - _archive_start;
    void *file_hash = cpio_get_file(_archive_start, cpio_len, (const char *)hash, &unused);
    uint8_t *print_hash_pointer = (uint8_t *)file_hash;

    /* If the file hash doesn't have a pointer, the file doesn't exist, so we cannot confirm the file is what we expect. Abort */
    if (file_hash == NULL) {
        printf("Cannot compare hashes for %s, expected hash, %s, doesn't exist\n", name, hash);
        abort();
    } else {

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
        if (strncmp((char *)file_hash, (char *)calculated_hash, hash_len) != 0) {
            printf("Hashes are different. Load failure\n");
            abort();
        }
    }

#endif  /* CONFIG_HASH_NONE */

    /* Print diagnostics. */
    printf("ELF-loading image '%s'\n", name);
    printf("  paddr=[%lx..%lx]\n", dest_paddr, dest_paddr + image_size - 1);
    printf("  vaddr=[%lx..%lx]\n", (vaddr_t)min_vaddr, (vaddr_t)max_vaddr - 1);
    printf("  virt_entry=%lx\n", (vaddr_t)elf_getEntryPoint(elf));

    /* Ensure the ELF file is valid. */
    if (elf_checkFile(elf) != 0) {
        printf("Attempting to load invalid ELF file '%s'.\n", name);
        abort();
    }

    /* Ensure sane alignment of the image. */
    if (!IS_ALIGNED(min_vaddr, PAGE_BITS)) {
        printf("Start of image '%s' is not 4K-aligned!\n", name);
        abort();
    }

    /* Ensure that we region we want to write to is sane. */
    ensure_phys_range_valid(name, dest_paddr, dest_paddr + image_size);

    /* Copy the data. */
    unpack_elf_to_paddr(elf, dest_paddr);

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
            phsize = ((struct Elf32_Header *)elf)->e_phentsize;
            source_paddr = (paddr_t)elf32_getProgramHeaderTable(elf);
        } else {
            phsize = ((struct Elf64_Header *)elf)->e_phentsize;
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
    return dest_paddr;
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
void load_images(struct image_info *kernel_info, struct image_info *user_info,
                 int max_user_images, int *num_images, void **dtb, uint32_t *dtb_size)
{
    int i;
    uint64_t kernel_phys_start, kernel_phys_end;
    uintptr_t dtb_phys_start, dtb_phys_end;
    paddr_t next_phys_addr;
    const char *elf_filename;
    unsigned long unused;
    unsigned long kernel_filesize;
    int has_dtb_cpio = 0;

    /* Load kernel. */
    unsigned long cpio_len = _archive_start_end - _archive_start;
    void *kernel_elf = cpio_get_file(_archive_start, cpio_len, "kernel.elf", &kernel_filesize);
    if (kernel_elf == NULL) {
        printf("No kernel image present in archive!\n");
        abort();
    }
    if (elf_checkFile(kernel_elf)) {
        printf("Kernel image not a valid ELF file!\n");
        abort();
    }

    elf_getMemoryBounds(kernel_elf, 1, &kernel_phys_start, &kernel_phys_end);

    if (dtb && !*dtb) {
        printf("Looking for DTB in CPIO archive...");
        /*
         * Note the lack of newline in the above printf().  Normally one would
         * have an fflush(stdout) here to ensure that the message shows up on a
         * line-buffered stream (which is the POSIX default on terminal
         * devices).  But we are freestanding (on the "bare metal"), and using
         * our own unbuffered printf() implementation.
         */
        *dtb = cpio_get_file(_archive_start, cpio_len, "kernel.dtb", &unused);
        if (*dtb == NULL) {
            printf("not found.\n");
        } else {
            has_dtb_cpio = 1;
            printf("found at %p.\n", *dtb);
        }
    }

    /*
     * Move the DTB out of the way, if it's present.
     */
    if (dtb && *dtb) {
        /* keep it page aligned */
        next_phys_addr = dtb_phys_start = ROUND_UP(kernel_phys_end, PAGE_BITS);

        *dtb_size = fdt_size(*dtb);
        if (!*dtb_size) {
            printf("Invalid device tree blob supplied!\n");
            abort();
        }

        /* Make sure this is a sane thing to do */
        ensure_phys_range_valid("DTB", next_phys_addr,
                                next_phys_addr + *dtb_size);

        memmove((void *)next_phys_addr, *dtb, *dtb_size);
        next_phys_addr += *dtb_size;
        next_phys_addr = ROUND_UP(next_phys_addr, PAGE_BITS);
        dtb_phys_end = next_phys_addr;

        printf("Loaded DTB from %p.\n", *dtb);
        printf("   paddr=[%lx..%lx]\n", dtb_phys_start, dtb_phys_end - 1);
        *dtb = (void *)dtb_phys_start;
    } else {
        next_phys_addr = ROUND_UP(kernel_phys_end, PAGE_BITS);
    }
    load_elf("kernel", kernel_elf,
             (paddr_t)kernel_phys_start, kernel_info, 0, kernel_filesize, "kernel.bin");

    /*
     * Load userspace images.
     *
     * We assume (and check) that the kernel is the first file in the archive,
     * that the DTB is the second if present,
     * and then load the (n+user_elf_offset)'th file in the archive onto the (n)'th CPU.
     */
    int user_elf_offset = 2;
    cpio_get_entry(_archive_start, cpio_len, 0, &elf_filename, &unused);
    if (strcmp(elf_filename, "kernel.elf") != 0) {
        printf("Kernel image not first image in archive.\n");
        abort();
    }
    cpio_get_entry(_archive_start, cpio_len, 1, &elf_filename, &unused);
    if (strcmp(elf_filename, "kernel.dtb") != 0) {
        if (has_dtb_cpio) {
            printf("Kernel DTB not second image in archive.\n");
            abort();
        }
        user_elf_offset = 1;
    }

#ifdef CONFIG_ELFLOADER_ROOTSERVERS_LAST
    /* work out the size of the user images - this corresponds to how much memory
     * load_elf uses */
    int total_user_image_size = 0;
    for (i = 0; i < max_user_images; i++) {
        void *user_elf = cpio_get_entry(_archive_start, cpio_len, i + user_elf_offset,
                                        &elf_filename, &unused);
        uint64_t min_vaddr, max_vaddr;
        total_user_image_size += rounded_image_size(user_elf, &min_vaddr, &max_vaddr);

        total_user_image_size += KEEP_HEADERS_SIZE;
    }

    /* work out where to place the user image */

    next_phys_addr = ROUND_DOWN(memory_region[0].end, PAGE_BITS) - ROUND_UP(total_user_image_size, PAGE_BITS);
#endif /* CONFIG_ELFLOADER_ROOTSERVERS_LAST */

    *num_images = 0;
    for (i = 0; i < max_user_images; i++) {
        /* Fetch info about the next ELF file in the archive. */
        void *user_elf = cpio_get_entry(_archive_start, cpio_len, i + user_elf_offset,
                                        &elf_filename, &unused);
        if (user_elf == NULL) {
            break;
        }

        /* Load the file into memory. */
        next_phys_addr = load_elf(elf_filename, user_elf,
                                  next_phys_addr, &user_info[*num_images], 1, unused, "app.bin");
        *num_images = i + 1;
    }
}

void __attribute__((weak)) platform_init(void) {}
