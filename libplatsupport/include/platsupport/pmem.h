/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <platsupport/io.h>

typedef enum pmem_type {
    PMEM_TYPE_RAM,
    PMEM_TYPE_UNKNOWN,
    PMEM_TYPE_DEVICE,
    PMEM_NUM_REGION_TYPES,
} pmem_type_t;

typedef struct pmem_region {
    pmem_type_t type;
    /* these specifically match the grub boot header struct definitions,
     * so must both be 64 bit on all systems */
    uint64_t base_addr;
    uint64_t length;
} pmem_region_t;

/*
 * Map a single pmem region.
 *
 * @param ops    to use,
 * @param region to unmap,
 * @param cached map the mappings cached or not,
 * @param        flags to pass through to mapper,
 * @return       vaddr the pmem is mapped to, NULL on failure.
 */
static inline void *ps_pmem_map(ps_io_ops_t *ops, pmem_region_t region, bool cached, ps_mem_flags_t flags)
{
    void *vaddr = ps_io_map(&ops->io_mapper, region.base_addr, region.length, cached, flags);
    if (vaddr == NULL) {
        ZF_LOGE("Failed to map paddr %p length %" PRIu64 "\n", (void *)(uintptr_t) region.base_addr, region.length);
    }
    return vaddr;
}

/*
 * Unmap a single pmem region.
 *
 * @param ops    to use,
 * @param region to unmap,
 * @param vaddr  the pmem_region is mapped to,
 * @param mapper to unmap with.
 */
static inline void ps_pmem_unmap(ps_io_ops_t *ops, pmem_region_t region, void *vaddr)
{
    ps_io_unmap(&ops->io_mapper, vaddr, region.length);
}
