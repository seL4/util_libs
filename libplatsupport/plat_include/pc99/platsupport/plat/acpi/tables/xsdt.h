/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma pack(push,1)

/* eXtended System Descriptor Table "XSDT" */
typedef struct acpi_xsdt {
    acpi_header_t  header;
    /* access via inline functions */
//    uint64_t        entry[];
} acpi_xsdt_t;

#pragma pack(pop)

/* retrieve the number of entries in an xsdt table */
static inline int acpi_xsdt_entry_count(acpi_xsdt_t *t)
{
    return (t->header.length - sizeof(*t)) / sizeof(uint64_t);
}

/* Retrieve the location of the first item in the list */
static inline uint64_t *acpi_xsdt_first(acpi_xsdt_t *hdr)
{
    return (uint64_t *)(hdr + 1);
}

/* Retrieve the location of the next item in the list */
static inline uint64_t *acpi_xsdt_next(acpi_xsdt_t *hdr, uint64_t *cur)
{
    char *next = (char *)(cur + 1);
    char *end = (char *)hdr + hdr->header.length;
    if (next < end) {
        return (uint64_t *)next;
    } else {
        return NULL;
    }
}
