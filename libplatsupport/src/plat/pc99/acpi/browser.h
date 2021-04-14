/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/plat/acpi/acpi.h>
#include "acpi.h"
#include "regions.h"

#ifdef CONFIG_LIB_SEL4_ACPI_DEBUG

/*
 * interactively browse the ACPI table at the given address
 * User input required to traverse the tree
 */
void
acpi_browse_tables(const acpi_rsdp_t *rsdp, size_t offset);

/*
 * interactively browse the tables given in the region list
 */
void
acpi_browse_regions(const RegionList_t *regions);

#else
static inline void acpi_browse_tables(const acpi_rsdp_t *rsdp, size_t offset)
{
    (void)rsdp;
    (void)offset;
}

static inline void acpi_browse_regions(const RegionList_t *regions)
{
    (void)regions;
}

#endif
