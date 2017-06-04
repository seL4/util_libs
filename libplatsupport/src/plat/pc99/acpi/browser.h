/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
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
acpi_browse_tables(const acpi_rsdp_t* rsdp, size_t offset);

/*
 * interactively browse the tables given in the region list
 */
void
acpi_browse_regions(const RegionList_t* regions);

#else
static inline void
acpi_browse_tables(const acpi_rsdp_t* rsdp, size_t offset)
{
    (void)rsdp;
    (void)offset;
}

static inline void
acpi_browse_regions(const RegionList_t* regions)
{
    (void)regions;
}

#endif
