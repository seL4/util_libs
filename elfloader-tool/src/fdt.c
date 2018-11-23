/*
 * Copyright 2019, Data61
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

#define FDT_MAGIC (0xd00dfeed)
/* Newest FDT version that we understand */
#define FDT_MAX_VER 17

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

uint32_t be32_to_le(uint32_t be)
{
    return ((be & 0xff) << 24) |
           ((be & 0xff00) << 8) |
           ((be & 0xff0000) >> 8) |
           ((be & 0xff000000) >> 24);
}

uint32_t fdt_size(void *fdt)
{
    struct fdt_header *hdr = (struct fdt_header *)fdt;

    if (be32_to_le(hdr->magic) != FDT_MAGIC ||
        be32_to_le(hdr->last_comp_version) > FDT_MAX_VER) {
        return 0;
    }

    return be32_to_le(hdr->totalsize);
}

